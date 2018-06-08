#include "CudaCFGFactory.hpp"

namespace Dyninst {
namespace ParseAPI {

Function *CudaCFGFactory::mkfunc(Address addr, FuncSource src, 
  std::string name, CodeObject * obj, CodeRegion * region, 
  Dyninst::InstructionSource * isrc) {
  // find function by name
  for (auto *function : _functions) {
    if (function->name == name) {
      Function *ret_func = new Function(addr, name, obj, region, isrc);
      //ret_func->_cache_valid = true;
      funcs_.add(*ret_func);

      bool first_entry = true;
      for (auto *block : function->blocks) {
        Block *ret_block = NULL;
        if (_block_filter.find(block->id) == _block_filter.end()) {
          ret_block = new Block(obj, region, block->insts[0]->offset);
          blocks_.add(*ret_block);
        } else {
          ret_block = _block_filter[block->id];
        }

        if (first_entry) {
          ret_func->setEntryBlock(ret_block);
          first_entry = false;
        }

        //ret_func->add_block(ret_block);

        for (auto *target : block->targets) {
          Block *ret_target_block = NULL;
          if (_block_filter.find(target->block->id) == _block_filter.end()) {
            ret_target_block = new Block(obj, region, target->block->insts[0]->offset);
            blocks_.add(*ret_target_block);
          } else {
            ret_target_block = _block_filter[target->block->id];
          }

          Edge *ret_edge = NULL;
          if (target->type == CudaParse::CALL) {
            ret_edge = new Edge(ret_block, ret_target_block, CALL);
            //ret_func->_call_edge_list.insert(ret_edge);
          } else {  // TODO(Keren): Add more edge types
            ret_edge = new Edge(ret_block, ret_target_block, DIRECT);
          }
          ret_edge->install();
          edges_.add(*ret_edge);
        }
      }
      return ret_func;
    }
  }
  return NULL;
  // iterate blocks
  // add blocks
  // iterate targets
  // add edges
}

}
}
