#include <CFGFactory.h>

namespace Dyninst {
namespace ParseAPI {

class PARSER_EXPORT CudaCFGFactory : public CFGFactory {   
 public:
    CudaCFGFactory() {};
    virtual ~CudaCFGFactory();

 protected:
    virtual Function * mkfunc(Address addr, FuncSource src, 
            std::string name, CodeObject * obj, CodeRegion * region, 
            Dyninst::InstructionSource * isrc);
    virtual Block * mkblock(Function * f, CodeRegion * r, 
            Address addr);
    virtual Edge * mkedge(Block * src, Block * trg, 
            EdgeTypeEnum type);
    virtual Block * mksink(CodeObject *obj, CodeRegion *r);

    virtual void free_func(Function * f);
    virtual void free_block(Block * b);
    virtual void free_edge(Edge * e);
};

}
}
