#include <dyn_regs.h>
#include <CodeSource.h>

#include "DotCFG.hpp"

namespace Dyninst {
namespace ParseAPI {

class PARSER_EXPORT CudaCodeSource : public CodeSource {
 public:
    CudaCodeSource(std::vector<CudaParse::Function *> &functions);
    ~CudaCodeSource() {};

 public:
    /** InstructionSource implementation **/
    virtual bool isValidAddress(const Address) const { return false; }
    virtual void* getPtrToInstruction(const Address) const { return NULL; }
    virtual void* getPtrToData(const Address) const { return NULL; }
    virtual unsigned int getAddressWidth() const { return 0; }
    virtual bool isCode(const Address) const { return false; }
    virtual bool isData(const Address) const { return false; }
    virtual bool isReadOnly(const Address) const { return false; }
    virtual Address offset() const { return 0; }
    virtual Address length() const { return 0; }
    virtual Architecture getArch() const { return Arch_cuda; }
};

}
}
