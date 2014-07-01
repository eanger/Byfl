/*
 * Simple cache model for predicting miss rates.
 *
 * By Eric Anger <eanger@lanl.gov>
 */

#include <algorithm>
#include <iterator>
#include <d4.h>

#include "byfl.h"

namespace bytesflops {}
using namespace bytesflops;
using namespace std;

class Cache {
  public:
    void access(uint64_t baseaddr, uint64_t numaddrs, uint64_t is_load);
    Cache(uint64_t line_size) : line_size_{line_size}, accesses_{0},
      split_accesses_{0} {
        d4cache* mem = d4new(NULL);
        d4cache* l3 = d4new(mem);
        d4cache* l2 = d4new(l3);
        dinero = d4new(l2);
        l3->name = "l3";
        l3->flags = D4F_CCC;
        l3->lg2blocksize = 6;
        l3->lg2subblocksize = 6;
        l3->lg2size = 24;
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
        l2->wbackf = wback_always;
        l2->name_replacement = "lru";
        l2->name_prefetch = "none";
        l2->name_walloc = "always";
        l2->name_wback = "always";
        dinero->name = "dinero";
        dinero->flags = D4F_CCC;
        dinero->lg2blocksize = 6;
        dinero->lg2subblocksize = 6;
        dinero->lg2size = 15;
        dinero->assoc = 8;
        dinero->replacementf = d4rep_lru;
        dinero->prefetchf = d4prefetch_none;
        dinero->wallocf = d4walloc_always;
        dinero->wbackf = wback_always;
        dinero->name_replacement = "lru";
        dinero->name_prefetch = "none";
        dinero->name_walloc = "always";
        dinero->name_wback = "always";
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
    d4cache* dinero;
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
  }

  // we've made all our accesses
  accesses_ += num_accesses;
  if(num_accesses != 1){
    ++split_accesses_;
  }

  d4memref ref;
  ref.address = baseaddr;
  ref.size = numaddrs;
  ref.accesstype = (is_load == 1 ? D4XREAD : D4XWRITE);
  d4ref(dinero, ref);
}

static Cache* cache = NULL;

namespace bytesflops{

void initialize_cache(void){
  cache = new Cache(bf_line_size);
}

// Access the cache model with this address.
void bf_touch_cache(uint64_t baseaddr, uint64_t numaddrs, uint64_t is_load){
  cache->access(baseaddr, numaddrs, is_load);
}

// Get cache accesses
uint64_t bf_get_cache_accesses(void){
  return cache->getAccesses();
}

// Get cache hits
vector<uint64_t> bf_get_cache_hits(void){
  // The total hits to a cache size N is equal to the sum of unique hits to 
  // all caches sized N or smaller.
  auto hits = cache->getHits();
  vector<uint64_t> tot_hits(hits.size());
  uint64_t prev_hits = 0;
  for(uint64_t i = 0; i < hits.size(); ++i){
    tot_hits[i] = hits[i] + prev_hits;
    prev_hits = tot_hits[i];
  }
  return tot_hits;
}

uint64_t bf_get_cold_misses(void){
  return cache->getColdMisses();
}

uint64_t bf_get_split_accesses(void){
  return cache->getSplitAccesses();
}

} // namespace bytesflops
