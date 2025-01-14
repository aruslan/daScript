#include "daScript/daScript.h"
#include "daScript/simulate/fs_file_info.h"

using namespace das;

TextPrinter tout;

void compile_and_run ( const string & fn, const string & mainFnName, bool outputProgramCode ) {
    auto access = make_shared<FsFileAccess>();
    ModuleGroup dummyGroup;
    if ( auto program = compileDaScript(fn,access,tout,dummyGroup) ) {
        if ( program->failed() ) {
            for ( auto & err : program->errors ) {
                tout << reportError(err.at, err.what, err.cerr );
            }
        } else {
            if ( outputProgramCode )
                tout << *program << "\n";
            Context ctx;
            program->simulate(ctx, tout);
            if ( auto fnTest = ctx.findFunction(mainFnName.c_str()) ) {
                ctx.restart();
                ctx.eval(fnTest, nullptr);
            } else {
                tout << "function '"  << mainFnName << " ' not found\n";
            }
        }
    }
}

void print_help() {
    tout << "daScript scriptName1 {scriptName2} .. {-main mainFnName} {-log}\n";
}

void require_project_specific_modules();//link time resolved dependencies

int main(int argc, const char * argv[]) {
    if ( argc<=1 ) {
        print_help();
        return -1;
    }
    vector<string> files;
    string mainName = "main";
    bool outputProgramCode = false;
    for ( int i=1; i < argc;  ) {
        if ( argv[i][0]=='-' ) {
            string cmd(argv[i]+1);
            if ( cmd=="main" ) {
                if (i+1 > argc)
                {
                    print_help();
                    return -1;
                }
                mainName = argv[i+1];
                i += 2;
            } else if ( cmd=="log" ) {
                outputProgramCode = true;
                i ++;
            } else {
                print_help();
                return -1;
            }
        } else {
            files.push_back(argv[i]);
            i ++;
        }
    }
    if (files.empty())
    {
        print_help();
        return -1;
    }
    // register modules
    NEED_MODULE(Module_BuiltIn);
    NEED_MODULE(Module_Math);
    NEED_MODULE(Module_Random);
    NEED_MODULE(Module_Rtti);
    require_project_specific_modules();
    // compile and run
    for ( const auto & fn : files ) {
        compile_and_run(fn, mainName, outputProgramCode);
    }
    // and done
    Module::Shutdown();
    return 0;
}


