#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/Decl.h"
#include "clang/AST/DeclBase.h"
#include "clang/AST/PrettyPrinter.h"
#include "clang/AST/TypeVisitor.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/AST/RecordLayout.h"
#include "clang/Basic/LangOptions.h"
#include "clang/Basic/LangStandard.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Frontend/FrontendOptions.h"
#include "clang/Tooling/ArgumentsAdjusters.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/AllTUsExecution.h"
#include "clang/Driver/Options.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Error.h"
#include "llvm/Support/Regex.h"
#include <cmath>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <memory>
#include <mutex>
#include <ostream>
#include <thread>
#include <utility>
#include <vector>

#include "clang/Frontend/ASTConsumers.h"
#include "llvm/Support/raw_ostream.h"

#include "data_encode.h"



using namespace clang;
using namespace clang::tooling;
using namespace llvm;

static cl::OptionCategory MyToolCategory("my-tool options");
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);
static cl::extrahelp MoreHelp("\nMore help text...\n");

static cl::opt<bool> ExtractALL(
    "extract-all",
    cl::desc("Extract all functions."),
    cl::init(false),cl::cat(MyToolCategory)
);

// static cl::opt<bool> FullInfo(
//     "full-info",
//     cl::desc("simple output"),
//     cl::init(false),cl::cat(MyToolCategory)
// );

static cl::opt<std::string> OutputFile(
    "output",
    cl::desc("output file path"),
    cl::init("./json.json"),cl::cat(MyToolCategory)
);

static cl::opt<int> Jobs(
    "jobs",
    cl::desc("jobs number"),
    cl::init(1),cl::cat(MyToolCategory)
);

static cl::opt<std::string> HostTriple(
    "host",
    cl::desc("host triple"),
    cl::init(""),cl::cat(MyToolCategory)
);

enum SHARE_T {
    Ignore,
    NoSHARED,
    SHARED,
};

static cl::opt<int> SHARE (
    "share",
    cl::desc("whether compile with -DSHARE.Ignore=0,NoSHARED=1,SHARED=2"),
    cl::init(Ignore),cl::cat(MyToolCategory)
);

namespace SA {
class jsonFileBuilder {
public:
    jsonFileBuilder() = default;
    jsonFileBuilder(const jsonFileBuilder &copy);
    RecordDecl *createRecordDecl(const clang::RecordDecl *Declaration);
    void addRecordDeclClang(clang::RecordDecl *Declaration);
    void addRecordDeclSA(SA::RecordDecl *Declaration) {
        File.addDecls(Declaration);
    }
    const multiset<RecordDecl*,jsonFile::RDCompare> &getDecls() const { return File.getDecls(); }
    jsonFile& getFile() { return File; }
    void dumpJsonFileTo(string path);
    void dumpJsonFileTo(ostream &os);
    void dump();
    bool operator==(const jsonFileBuilder &rhs) const;
private:
    jsonFile File;
    string path;
};

RecordDecl *jsonFileBuilder::createRecordDecl(const clang::RecordDecl *Declaration) {
    auto& ASTContext = Declaration->getASTContext();
    bool hasLayout = false;
    if(Declaration->getDefinition() != nullptr && !Declaration->isInvalidDecl() && Declaration->isCompleteDefinition()) {
        hasLayout = true;
    }
    // auto& Layout = ASTContext.getASTRecordLayout(Declaration);
    string TypeName = Declaration->getNameAsString();
    if(TypeName == "") {
        if(auto ptr = Declaration->getTypedefNameForAnonDecl()) {
            TypeName = ptr->getNameAsString();
        } else {
            // TypeName = "Error_Type";
        }
    }
    RecordType_t RecordType = RecordType_t(Declaration->isStruct());
    // errs()<<TypeName << " "<< int(RecordType) <<"\n";
    RecordDecl *Decl = new RecordDecl(TypeName,RecordType);
    if(hasLayout) {
        auto& Layout = ASTContext.getASTRecordLayout(Declaration);
        auto chars = Layout.getSize();
        Decl->setSize(ASTContext.toBits(chars));
    }
    int i = 0;
    for(auto field : Declaration->fields()) {
        // field->dump(errs());
        string FieldName = field->getNameAsString();
        PrintingPolicy Policy = Declaration->getASTContext().getPrintingPolicy();
        Policy.AnonymousTagLocations = false;
        
        string TypeName = field->getType().getAsString(Policy);
        if(TypeName.find("(unnamed") != string::npos) {
            TypeName.insert(TypeName.find("unnamed"),"#");
        } else if(TypeName.find("(anonymous") != string::npos) {
            TypeName.insert(TypeName.find("anonymous"),"#");
        }
        // errs() << format("FieldName %s; TypeName %s; Anon %d; isArray %d\n",FieldName.c_str(),TypeName.c_str(),field->isAnonymousStructOrUnion(),field->getType().getTypePtr()->isArrayType());
        Field *f = new Field(FieldName,TypeName,"");
        if(f->isAnon()) {
            // errs()<<"isAnon"<<"\n";
            SplitQualType SQT = field->getType().split();
            // errs()<<"Qualifiers "<<SQT.Quals.getAsString()<<"\n";
            // SQT.Ty->isUnionType();
            if(SQT.Ty->isArrayType()) {
                f->setAnonDecl(createRecordDecl(SQT.Ty->getArrayElementTypeNoTypeQual()->getAsRecordDecl()));
            } else if(SQT.Ty->isRecordType() || SQT.Ty->isUnionType()) {
                f->setAnonDecl(createRecordDecl(SQT.Ty->getAsRecordDecl()));
            } else {
                // assert(0 && "meet unexpected type");
                errs() <<format("skip enum in %s\n",TypeName.c_str());
                delete f;
                continue;
            }
        }
        f->setSize(ASTContext.getTypeSize(field->getType()));
        if(hasLayout) {
            auto& Layout = ASTContext.getASTRecordLayout(Declaration);
            f->setOffset(Layout.getFieldOffset(i++));
        }
        Decl->appendField(f);
    }
    return Decl;
}

jsonFileBuilder::jsonFileBuilder(const jsonFileBuilder &copy)
    :File(copy.File),path(copy.path) {
}


void jsonFileBuilder::addRecordDeclClang(clang::RecordDecl *Declaration) {
    // 判断是否是forward declaration
    if(Declaration->getDefinition() == nullptr) {
        return;
    }
    errs() << "not complete declaration:" << Declaration->isCompleteDefinition() << " " << Declaration->getName() << "\n";
    SA::RecordDecl* decl = createRecordDecl(Declaration);
    if(decl->isAnon()) {
        delete decl;
        return;
    }
    if(decl->isEmpty()) {
        errs() << "empty record:"<<decl->getTypeName()<<"\n";
        delete decl;
        return;
    }

    // if(File.getDecls().count(decl)) {
    //     // errs() << "duplicate record\n";
    //     return;
    // }

    errs() << "try add record:"<<decl->getTypeName()<<"\n";
    auto [begin,end] = File.getDecls().equal_range(decl);
    for(auto it = begin; it != end; it++) {
        if(**it == *decl) {
            errs() << "has the same before\n"; 
            return;
        }
    }
    File.addDecls(decl);
}

void jsonFileBuilder::dumpJsonFileTo(string path= "./json.json") {
    std::ofstream os(path);
    // os << "start output\n";
    os.flush();
    json j = File;
    errs() << format("size : %d %d\n",j.size(),File.getDecls().size());

    j.dump(4);
    os<<std::setw(4)<<j<<std::endl;
    os.flush();
    os.close();
}

void jsonFileBuilder::dumpJsonFileTo(ostream &os) {
    json j = File;
    os << std::setw(4) << j << std::endl;
}

void jsonFileBuilder::dump() {
    File.dumpJsonFile();
}

bool jsonFileBuilder::operator==(const jsonFileBuilder &rhs) const {
    return this->File == rhs.File;
};

} //end of SA


SA::jsonFileBuilder Builder;
std::set<std::string> FoundDecl;
std::mutex mu_builder, mu_founddecl,mu_Tool;

class FindNamedClassVisitor : public RecursiveASTVisitor<FindNamedClassVisitor> {
public:
    explicit FindNamedClassVisitor(ASTContext *Context) :
        Context(Context) {}

    bool VisitRecordDecl(RecordDecl *Declaration) {
        std::string Name = Declaration->getNameAsString();
        if(Name == "") {
            if(auto ptr = Declaration->getTypedefNameForAnonDecl()) {
                Name = ptr->getNameAsString();
            }
        }
        if(Name == "") return true;

        // {
        //     std::unique_lock<std::mutex> L(mu_founddecl);
        //     if(FoundDecl.count(Name)) {
        //         return true;
        //     }
        //     FoundDecl.insert(Name);
        // }

        // Declaration->dump(errs());
        if(Declaration->isEnum()) return true;
        if(!Declaration->isAnonymousStructOrUnion()) {
            std::unique_lock<std::mutex> L(mu_builder);
            Builder.addRecordDeclClang(Declaration);
        }
        return true;
    }

    bool VisitTypedefDecl(TypedefDecl *Declaration) {
        // llvm::outs()<<"typedef:\n";
        // Declaration->dumpColor();
        // llvm::outs()<<"typedef getUnderlyingDecl:\n";
        // Declaration->getUnderlyingDecl()->dumpColor();
        // llvm::outs()<<"typedef getFirstDecl:\n";
        // Declaration->getFirstDecl()->dumpColor();
        // llvm::outs()<<"typedef typesourceinfo:\n";
        // auto srcInfo = Declaration->getTypeSourceInfo()->getType();
        // llvm::outs()<<"typedef gettypefordecl:\n";
        // Declaration->getTypeForDecl();
        // llvm::outs()<<"typeclass name : "<<std::string(srcInfo->getTypeClassName())<<'\n';
        // Declaration->getTypeSourceInfo()->getType().dump();
        // llvm::outs()<<"getQualifiedNameAsString:"<<Declaration->getQualifiedNameAsString()<<'\n';
        auto QT = Declaration->getTypeSourceInfo()->getType();
        if(QT->isRecordType() && QT->getAsRecordDecl()->getDefinition() == nullptr) {
            errs() << "forward declaration : " << QT->getTypeClassName() << "\n";
            return true;
        }
        SplitQualType SQT = QT.split();
        // errs()<<"splitqualtype:" << SQT.Ty << "\n";
        // TypeVisitor ;
        // SQT.Ty->dump();
        if(auto RD = SQT.Ty->getAsRecordDecl()) {
            if(RD->getNameAsString() == "") {
                return true;
            }
            if(RD->getNameAsString() == Declaration->getNameAsString()) {
                return true;
            }
        }
        string DeclName = Declaration->getNameAsString();
        SA::RecordDecl *Decl = new SA::RecordDecl(DeclName,SA::Typedef);
        PrintingPolicy Policy = Declaration->getASTContext().getPrintingPolicy();
        string TypeName = QT.getAsString(Policy);
        if(TypeName.find("(unnamed") != string::npos) {
            TypeName.insert(TypeName.find("unnamed"),"#");
        } else if(TypeName.find("(anonymous") != string::npos) {
            TypeName.insert(TypeName.find("anonymous"),"#");
        }
        // errs() << format("FieldName %s; TypeName %s; Anon %d; isArray %d\n",FieldName.c_str(),TypeName.c_str(),field->isAnonymousStructOrUnion(),field->getType().getTypePtr()->isArrayType());
        SA::Field *f = new SA::Field("typedef_field",TypeName,"");
        if(f->isAnon()) {
            // errs()<<"isAnon"<<"\n";
            // SplitQualType SQT = field->getType().split();
            // errs()<<"Qualifiers "<<SQT.Quals.getAsString()<<"\n";
            SQT.Ty->isUnionType();
            if(SQT.Ty->isArrayType()) {
                f->setAnonDecl(Builder.createRecordDecl(SQT.Ty->getArrayElementTypeNoTypeQual()->getAsRecordDecl()));
            } else if(SQT.Ty->isRecordType() || SQT.Ty->isUnionType()) {
                f->setAnonDecl(Builder.createRecordDecl(SQT.Ty->getAsRecordDecl()));
            } else {
                // assert(0 && "meet unexpected type");
                errs() <<format("skip enum in %s\n",TypeName.c_str());
                delete f;
                f = nullptr;
            }
        }
        if(f) {
            Decl->appendField(f);
        }
        errs() << "try add record:"<<DeclName<<"\n";
        auto [begin,end] = Builder.getDecls().equal_range(Decl);
        for(auto it = begin; it != end; it++) {
            if(**it == *Decl) {
                errs() << "has the same before\n"; 
                return true;
            }
        }
        //设置decl的大小
        Decl->setSize(Declaration->getASTContext().getTypeSize(QT));
        Builder.addRecordDeclSA(Decl);
        // while(SQT != SQT.getSingleStepDesugaredType()){
        //     SQT = SQT.getSingleStepDesugaredType();
        //     // SQT.Ty->dump(errs(),*Context);
        //     std::string str = QualType::getAsString(SQT,Context->getPrintingPolicy());
        //     // llvm::errs()<<str<<"\n";
        //     if(SQT.Ty->isEnumeralType()) break;
        //     if(auto rec_decl = SQT.Ty->getAsRecordDecl()) {
        //         if(!rec_decl->isAnonymousStructOrUnion()) {
        //             VisitRecordDecl(rec_decl);
        //             continue;
        //         }
        //         int before = rec_decl->isAnonymousStructOrUnion();
        //         rec_decl->setTypedefNameForAnonDecl(Declaration);
        //         int after = rec_decl->isAnonymousStructOrUnion();
        //         errs() << format("name:%s, before:%d, after:%d",rec_decl->getTypedefNameForAnonDecl()->getNameAsString().c_str(),before,after);
        //         // VisitCXXRecordDecl(rec_decl);
        //         VisitRecordDecl(rec_decl);
        //         break;
        //     }
        // }
        
        // Declaration->getTypeForDecl()->dump();
        // auto ptr = static_cast<CXXRecordDecl*>(Declaration->getUnderlyingDecl());
        // if(ptr) ptr->dumpAsDecl();

        // if(auto ptr = Declaration->getAnonDeclWithTypedefName()) ptr->dumpAsDecl();
        // VisitCXXRecordDecl(static_cast<CXXRecordDecl*>(Declaration->getUnderlyingDecl()));
        return true;
    }

    bool shouldVisitImplicitCode() const {return true;}

private:
    ASTContext *Context;
};


class FindNamedClassConsumer : public clang::ASTConsumer {
public:
    explicit FindNamedClassConsumer(ASTContext *Context) : Visitor(Context) {}

    void HandleTranslationUnit(clang::ASTContext &Context) override {
        auto D = Context.getVaListTagDecl();
        if(D->getLexicalDeclContext() == static_cast<DeclContext*>(Context.getTranslationUnitDecl())) {
            Context.getTranslationUnitDecl()->addDecl(D);
        }
        Visitor.TraverseDecl(Context.getTranslationUnitDecl());
    }

private:
    FindNamedClassVisitor Visitor;
};

std::atomic<int> file_count;

class FindNamedClassAction : public clang::ASTFrontendAction {
public:
    // explicit FindNamedClassAction(jsonFileBuidler &Builder):ASTFrontendAction(),Builder(Builder) {}
    std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(clang::CompilerInstance &Compiler, llvm::StringRef InFile) override {
        return std::make_unique<FindNamedClassConsumer>(&Compiler.getASTContext());
    }

    bool PrepareToExecuteAction(CompilerInstance &CI) override{
        if(HostTriple != "") {
            CI.getInvocation().TargetOpts->Triple = HostTriple;
            CI.getInvocation().TargetOpts->CPU="";
        }
        
        return true;
    }

    bool BeginSourceFileAction(CompilerInstance &CI) override {
        // 排除c++的情况，方式出现奇怪的va_list
        auto lang = LangStandard::getLangStandardForKind(CI.getLangOpts().LangStd);
        if(lang.isCPlusPlus()) {
            errs() << format("meets cpp files : %s\n",getCurrentFile().bytes_begin());
            return false;
        }
        if(SHARE == Ignore) {
            // mu_Tool.unlock();
            return true;
        }
        const auto &args = CI.getInvocation().getCC1CommandLine();
        bool HasShare = std::find(args.begin(),args.end(),"SHARED") != args.end();
        if((SHARE == SHARED) ^ HasShare) {
            return false;
        }
        // mu_Tool.unlock();
        return true;
    }

    void EndSourceFileAction() override {
        // mu_Tool.lock();
    }

private:
    // std::map<std::string >
};

#if false
template<typename T>
class ConcurrentAction {
public:
ConcurrentAction(int argc,const char **argv)
    :argc(argc),argv(argv),FileMeeted(0) {}
void start(int jobs, std::vector<std::string> sources) {
    jobs = sources.size() < jobs? sources.size(): jobs;
    int total = sources.size();
    int task_size = total / jobs;

    //如果task_size过大或者jobs==1，则一个文件一个文件进行操作
    if(task_size > 1000) {
        jobs = 1;
    }
    if(jobs == 1) {
        auto ExpectedParser = CommonOptionsParser::create(argc,argv,MyToolCategory);
        if(!ExpectedParser) {
            errs() << ExpectedParser.takeError();
            return;
        }
        CommonOptionsParser& OptionsParser = ExpectedParser.get();
        for(int i = 0; i < total; i++) {
            std::string Target = sources[i];
            int now = FileMeeted.fetch_add(1);
            errs() << format("[%d/%d]:TargetFile %s\n",now,sources.size(),Target.c_str());
            std::flush(cerr);
            mu_Tool.lock();
            ClangTool Tool(OptionsParser.getCompilations(),Target);
            Tool.run(newFrontendActionFactory<T>().get());
            mu_Tool.unlock();
        }
        return;
    }
    //实验性的多线程
    std::vector<std::thread> workers;
    for(int i = 0; i < jobs; i++) {
        if(i == jobs - 1) {
            workers.emplace_back(std::thread(WorkerFunc,i * task_size, sources.size() - i * task_size,sources,this));
        } else {
            workers.emplace_back(std::thread(WorkerFunc,i * task_size, task_size,sources,this));
        }
    }
    for(auto &worker : workers) {
        worker.join();
    }
}

static void WorkerFunc(int start, int len, std::vector<std::string> sources,ConcurrentAction<T> *CA) {
    errs() << format("[start,end): [%d,%d)\n",start,start+len);
    mu_Tool.lock();
    auto ExpectedParser = CommonOptionsParser::create(CA->argc,CA->argv,MyToolCategory);
    mu_Tool.unlock();
    if(!ExpectedParser) {
        errs() << ExpectedParser.takeError();
        return;
    }
    CommonOptionsParser& OptionsParser = ExpectedParser.get();
    std::vector<std::string> Targets(sources.begin() + start,sources.begin() + start + len);
    errs() << "Targets size:" << Targets.size() << "\n";
    mu_Tool.lock();
    ClangTool Tool(OptionsParser.getCompilations(),Targets);
    Tool.run(newFrontendActionFactory<T>().get());
    mu_Tool.unlock();

};

private:
    int argc;
    const char **argv;
    // CommonOptionsParser &OptionsParser;
    std::atomic<int> FileMeeted;
};
#endif


int main(int argc, const char **argv) {
    // argc=1;
    auto ExpectedParser = CommonOptionsParser::create(argc,argv,MyToolCategory);
    // auto ExpectedParser1 = CommonOptionsParser::create(argc,argv+1,MyToolCategory);
    if(!ExpectedParser) {
        llvm::errs() << ExpectedParser.takeError();
        return 1;
    }
    CommonOptionsParser& OptionsParser = ExpectedParser.get();
    std::vector<std::string> Sources = OptionsParser.getSourcePathList();
    if(ExtractALL) {
        Sources = OptionsParser.getCompilations().getAllFiles();
    }
    auto FrontendActionFactory = newFrontendActionFactory<FindNamedClassAction>();

    AllTUsToolExecutor Executor(OptionsParser.getCompilations(),Jobs);
    llvm::Error error = Executor.execute({make_pair(std::move(FrontendActionFactory),ArgumentsAdjuster())});
    if(error) {
        // llvm::errs() << error;
        auto err = handleErrors(std::move(error), [](const StringError &err){});
        assert(!err);
        // return 1;
    }

    // for(auto Target : Sources) {
    //     errs() << format("[%d/%d]:TargetFile %s\n",count++,Sources.size(),Target.c_str());
    //     ClangTool Tool(OptionsParser.getCompilations(),Target);
    //     Tool.run(FrontendActionFactory.get());
    // }

    // ConcurrentAction<FindNamedClassAction> concurrent_action(argc,argv);
    // concurrent_action.start(Jobs,Sources);

    Builder.dump();
    Builder.dumpJsonFileTo(OutputFile);
    return 0;
}
