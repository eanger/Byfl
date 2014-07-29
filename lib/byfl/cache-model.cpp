/*
 * Simple cache model for predicting miss rates.
 *
 * By Eric Anger <eanger@lanl.gov>
 */

#include <algorithm>
#include <iterator>
extern "C"{
#include <d4.h>
}
#include <thread>
#include <mutex>

#include "byfl.h"

namespace bytesflops {}
using namespace bytesflops;
using namespace std;

class Cache {
  public:
    void access(uint64_t baseaddr, uint64_t numaddrs, uint64_t is_load);
    Cache(uint64_t line_size) : line_size_{line_size}, accesses_{0},
      split_accesses_{0} {
        mem = d4new(NULL);
        l3 = d4new(mem);
        l2 = d4new(l3);
        l1 = d4new(l2);
        l3->name = "l3";
        l3->flags = D4F_CCC;
        l3->lg2blocksize = 6;
        l3->lg2subblocksize = 6;
        l3->lg2size = 21;
        l3->assoc = 16;
        l3->replacementf = d4rep_lru;
        l3->prefetchf = d4prefetch_none;
        l3->wallocf = d4walloc_always;
        l3->wbackf = d4wback_always;
        l3->name_replacement = "lru";
        l3->name_prefetch = "none";
        l3->name_walloc = "always";
        l3->name_wback = "always";
        l2->name = "l2";
        l2->flags = D4F_CCC;
        l2->lg2blocksize = 6;
        l2->lg2subblocksize = 6;
        l2->lg2size = 18;
        l2->assoc = 8;
        l2->replacementf = d4rep_lru;
        l2->prefetchf = d4prefetch_none;
        l2->wallocf = d4walloc_always;
        l2->wbackf = d4wback_always;
        l2->name_replacement = "lru";
        l2->name_prefetch = "none";
        l2->name_walloc = "always";
        l2->name_wback = "always";
        l1->name = "l1";
        l1->flags = D4F_CCC;
        l1->lg2blocksize = 6;
        l1->lg2subblocksize = 6;
        l1->lg2size = 15;
        l1->assoc = 8;
        l1->replacementf = d4rep_lru;
        l1->prefetchf = d4prefetch_none;
        l1->wallocf = d4walloc_always;
        l1->wbackf = d4wback_always;
        l1->name_replacement = "lru";
        l1->name_prefetch = "none";
        l1->name_walloc = "always";
        l1->name_wback = "always";
        if(d4setup() != 0){
          exit(-1);
        }
      }
    uint64_t getAccesses() const { return accesses_; }
    vector<uint64_t> getHits() const { return hits_; }
    uint64_t getColdMisses() const { return hits_.size(); }
    uint64_t getSplitAccesses() const { return split_accesses_; }

  private:
    vector<uint64_t> lines_; // back is mru, front is lru
    uint64_t line_size_;
    uint64_t accesses_;
    vector<uint64_t> hits_;  // back is lru, front is mru
    uint64_t split_accesses_;
  public:
    d4cache* l1, *l2, *l3, *mem;
};

void Cache::access(uint64_t baseaddr, uint64_t numaddrs, uint64_t is_load){
  uint64_t num_accesses = 0; // running total of number of lines accessed
  for(uint64_t addr = baseaddr / line_size_ * line_size_;
      addr <= (baseaddr + numaddrs ) / line_size_ * line_size_;
      addr += line_size_){
    ++num_accesses;
    auto line = lines_.rbegin();
    auto hit = begin(hits_);
    bool found = false;
    for(; line != lines_.rend(); ++line, ++hit){
      if(addr == *line){
        found = true;
        ++(*hit);
        // erase the line pointed to by this reverse iterator. see
        // stackoverflow.com/questions/1830158/how-to-call-erase-with-a-reverse-iterator
        lines_.erase((line + 1).base());
        break;
      }
    }

    if(!found){
      // add a new hit entry
      hits_.push_back(0);
    }

    // move up this address to mru position
    lines_.push_back(addr);

    // do dinero access
    d4memref ref;
    ref.address = addr;
    ref.size = line_size_;
    ref.accesstype = (is_load == 1 ? D4XREAD : D4XWRITE);
    d4ref(l1, ref);
  }

  // we've made all our accesses
  accesses_ += num_accesses;
  if(num_accesses != 1){
    split_accesses_ += num_accesses;
  }
}

namespace bytesflops{

static __thread Cache* cache = nullptr;
static vector<Cache*>* caches = nullptr;
static mutex mymutex;

void initialize_cache(void){
  if(caches == nullptr){
    caches = new vector<Cache*>();
  }
}

// Access the cache model with this address.
void bf_touch_cache(uint64_t baseaddr, uint64_t numaddrs, uint64_t is_load){
  if(cache == nullptr){
    // Only let one thread update caches at a time.
    lock_guard<mutex> guard(mymutex);
    cache = new Cache(bf_line_size);
    caches->push_back(cache);
  }
  cache->access(baseaddr, numaddrs, is_load);
}

// Get cache accesses
uint64_t bf_get_cache_accesses(void){
  uint64_t res = 0;
  for(auto& cache: *caches){
    res += cache->getAccesses();
  }
  return res;
}

// Get cache hits
vector<uint64_t> bf_get_cache_hits(void){
  // hack: dump out our stats now
  long accesses[4]{0,0,0,0};
  long conflicts[3]{0,0,0};
  for(auto& cache: *caches){
    auto l1 = cache->l1;
    auto l2 = cache->l2;
    auto l3 = cache->l3;
    accesses[0] += l1->fetch[D4XREAD] + l1->fetch[D4XWRITE];
    accesses[1] += l2->fetch[D4XREAD] + l2->fetch[D4XWRITE];
    accesses[2] += l3->fetch[D4XREAD] + l3->fetch[D4XWRITE];
    accesses[3] += l3->miss[D4XREAD] + l3->miss[D4XWRITE];
    conflicts[0] += l1->conf_miss[D4XREAD] + l1->conf_miss[D4XWRITE];
    conflicts[1] += l2->conf_miss[D4XREAD] + l2->conf_miss[D4XWRITE];
    conflicts[2] += l3->conf_miss[D4XREAD] + l3->conf_miss[D4XWRITE];
  }
  ofstream df{"dinero.dump"};
  df << fixed;
  df << "l1 accesses\tl2 accesses\tl3 accesses\tmem accesses" << endl;
  df << accesses[0] << "\t"
     << accesses[1] << "\t"
     << accesses[2] << "\t"
     << accesses[3] << endl;
  df << "l1 conflict misses\tl2 conflict misses\tl3 conflict misses" << endl;
  df << conflicts[0] << "\t"
     << conflicts[1] << "\t"
     << conflicts[2] << endl;
  // The total hits to a cache size N is equal to the sum of unique hits to all
  // caches sized N or smaller.  We'll aggregate the cache performance across
  // all threads; global L1 accesses is equivalent to the sum of individual L1
  // accesses, etc.
  size_t longest = 0;
  for(auto& cache: *caches){
    auto cur_size = cache->getHits().size();
    if(cur_size > longest){
      longest = cur_size;
    }
  }
  // Initialize all to zero. As long as the longest thread cache.
  vector<uint64_t> tot_hits(longest, 0);
  for(auto& cache: *caches){
    auto hits = cache->getHits();
    for(uint64_t i = 0; i < hits.size(); ++i){
      tot_hits[i] += hits[i];
    }
  }

  // Compute rolling sum of all previous entries.
  uint64_t prev_hits = 0;
  for(uint64_t i = 0; i < tot_hits.size(); ++i){
    tot_hits[i] += prev_hits;
    prev_hits = tot_hits[i];
  }
  return tot_hits;
}

uint64_t bf_get_cold_misses(void){
  uint64_t res = 0;
  for(auto& cache: *caches){
    res += cache->getColdMisses();
  }
  return res;
}

uint64_t bf_get_split_accesses(void){
  uint64_t res = 0;
  for(auto& cache: *caches){
    res += cache->getSplitAccesses();
  }
  return res;
}

} // namespace bytesflops
