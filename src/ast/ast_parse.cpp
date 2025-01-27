#include "daScript/misc/platform.h"

#include "daScript/ast/ast.h"

void das_yybegin(const char * str);
int das_yyparse();
int das_yylex_destroy();

namespace das {

    vector<string> getAllRequie ( const char * src, uint32_t length ) {
        vector<string> req;
        const char * src_end = src + length;
        bool wb = true;
        while ( src < src_end ) {
            if ( src[0]=='"' ) {
                src ++;
                while ( src < src_end && src[0]!='"' ) {
                    src ++;
                }
                src ++;
                wb = true;
                continue;
            } else if ( src[0]=='/' && src[1]=='/' ) {
                while ( src < src_end && !(src[0]=='\n') ) {
                    src ++;
                }
                src ++;
                wb = true;
                continue;
            } else if ( src[0]=='/' && src[1]=='*' ) {
                int depth = 0;
                while ( src < src_end ) {
                    if ( src[0]=='/' && src[1]=='*' ) {
                        depth ++;
                        src += 2;
                    } else if ( src[0]=='*' && src[1]=='/' ) {
                        if ( --depth==0 ) break;
                        src += 2;
                    } else {
                        src ++;
                    }
                }
                src +=2;
                wb = true;
                continue;
            } else if ( wb && src[0]=='r' ) {
                if ( memcmp(src, "require", 7)==0 ) {
                    src += 7;
                    if ( isspace(src[0]) ) {
                        while ( src < src_end && isspace(src[0]) ) {
                            src ++;
                        }
                        if ( src >= src_end ) {
                            continue;
                        }
                        if ( src[0]=='_' || isalpha(src[0]) ) {
                            string mod;
                            while ( src < src_end && (isalnum(src[0]) || src[0]=='_') ) {
                                mod += *src ++;
                            }
                            req.push_back(mod);
                            continue;
                        } else {
                            wb = true;
                            goto nextChar;
                        }
                    } else {
                        wb = false;
                        goto nextChar;
                    }
                } else {
                    goto nextChar;
                }
            }
        nextChar:
            wb = src[0]!='_' && (wb ? !isalnum(src[0]) : !isalpha(src[0]));
            src ++;
        }
        return req;
    }

    bool getPrerequisits ( const string & fileName, const FileAccessPtr & access, vector<string> & req, vector<string> & missing, ModuleGroup & libGroup) {
        if ( auto fi = access->getFileInfo(fileName) ) {
            vector<string> ownReq = getAllRequie(fi->source, fi->sourceLength);
            for ( auto & mod : ownReq ) {
                auto module = Module::require(mod);
                if ( !module ) {
                    if ( find(req.begin(), req.end(), mod)==req.end() ) {
                        req.push_back(mod);
                        // module file name
                        string modFn = access->getIncludeFileName(fileName, mod) + ".das";
                        if ( !getPrerequisits(modFn, access, req, missing, libGroup) ) {
                            return false;
                        }
                    }
                } else {
                    libGroup.addModule(module);
                }
            }
            return true;
        } else {
            missing.push_back(fileName);
            return false;
        }
    }

    // PARSER

    ProgramPtr g_Program;
    FileAccessPtr g_Access;
    vector<FileInfo *>  g_FileAccessStack;

    extern "C" int64_t ref_time_ticks ();
    extern "C" int get_time_usec (int64_t reft);

    ProgramPtr parseDaScript ( const string & fileName, const FileAccessPtr & access, TextWriter & logs, ModuleGroup & libGroup, bool exportAll ) {
        auto time0 = ref_time_ticks();
        int err;
        auto program = g_Program = make_shared<Program>();
        g_Access = access;
        program->thisModuleGroup = &libGroup;
        libGroup.foreach([&](Module * pm){
            g_Program->library.addModule(pm);
            return true;
        },"*");
        g_FileAccessStack.clear();
        if ( auto fi = access->getFileInfo(fileName) ) {
            g_FileAccessStack.push_back(fi);
            das_yybegin(fi->source);
        } else {
            g_Program->error(fileName + " not found", LineInfo());
            g_Program.reset();
            g_Access.reset();
            g_FileAccessStack.clear();
            return program;
        }
        err = das_yyparse();        // TODO: add mutex or make thread safe?
        das_yylex_destroy();
        g_Program.reset();
        g_Access.reset();
        g_FileAccessStack.clear();
        if ( err || program->failed() ) {
            sort(program->errors.begin(),program->errors.end());
            return program;
        } else {
            program->inferTypes(logs);
            if ( !program->failed() ) {
                program->lint();
                if (program->options.getOption("optimize", true)) {
                    program->optimize(logs);
                } else {
                    program->buildAccessFlags(logs);
                }
                if (!program->failed())
                    program->verifyAndFoldContracts();
                if (!program->failed())
                    program->markOrRemoveUnusedSymbols(exportAll);
                if (!program->failed())
                    program->allocateStack(logs);
                if (!program->failed())
                    program->finalizeAnnotations();
            }
            if (!program->failed()) {
                if (program->options.getOption("log")) {
                    logs << *program;
                }
                if (program->options.getOption("plot")) {
                    logs << "\n" << program->dotGraph() << "\n";
                }
            }
            sort(program->errors.begin(), program->errors.end());
            if ( program->options.getOption("logCompileTime",false) ) {
                auto dt = get_time_usec(time0) / 1000000.;
                logs << "compiler took " << dt << "\n";
            }
            return program;
        }
    }

    ProgramPtr compileDaScript ( const string & fileName, const FileAccessPtr & access, TextWriter & logs, ModuleGroup & libGroup, bool exportAll ) {
        vector<string> req, missing;
        if ( getPrerequisits(fileName, access, req, missing, libGroup) ) {
            reverse(req.begin(), req.end());
            for ( auto & mod : req ) {
                if ( !libGroup.findModule(mod) ) {
                    string modFn = access->getIncludeFileName(fileName, mod) + ".das";
                    auto program = parseDaScript(modFn, access, logs, libGroup, true);
                    if ( program->failed() ) {
                        return program;
                    }
                    program->thisModule->name = mod;
                    libGroup.addModule(program->thisModule.release());
                    program->library.foreach([&](Module * pm) -> bool {
                        if ( !pm->name.empty() && pm->name!="$" ) {
                            if ( !libGroup.findModule(pm->name) ) {
                                libGroup.addModule(pm);
                            }
                        }
                        return true;
                    }, "*");
                }
            }
            return parseDaScript(fileName, access, logs, libGroup, exportAll);
        } else {
            auto program = make_shared<Program>();
            program->thisModuleGroup = &libGroup;
            for ( auto & mis : missing ) {
                program->error("missing prerequisit " + mis, LineInfo());
            }
            return program;
        }
    }
}

