import qbs

Module {
    FileTagger {
        patterns: "*.ld"
        fileTags: ["linkerscript"]
    }

    Rule {
        id: hex
        inputs: ["application"]
        Artifact {
            fileTags: ["hex"]
            filePath: input.fileName + ".hex"
        }

        prepare: {
            var args = ["-O", "ihex", input.filePath, output.filePath];
            var cmd = new Command(product.cpp.objcopyPath, args);
            cmd.description = "generating " + output.fileName;
            return cmd;
        }
    }

    Rule {
        id: bin
        inputs: ["application"]
        Artifact {
            fileTags: ["bin"]
            filePath: input.fileName + ".bin"
        }

        prepare: {
            var args = ["-O", "binary", input.filePath, output.filePath];
            var cmd = new Command(product.cpp.objcopyPath, args);
            cmd.description = "generating " + output.fileName;
            return cmd;
        }
    }

    Rule {
        id: size
        inputs: ["application"]
        alwaysRun: true
        Artifact {
            fileTags: ["size"]
        }

        prepare: {
            var args = [input.filePath, '-B'];
            var cmd = new Command(product.cpp.objcopyPath.replace("objcopy", "size"), args);
            cmd.silent = true;
            return cmd;
        }
    }

    Rule {
        id: map
        inputs: ["application"]
        Artifact {
            fileTags: ["map"]
            filePath: input.fileName + ".map"
        }

        prepare: {
            var args = ['-f', 'sysv', '--size-sort', '-r', input.filePath];
            var cmd = new Command(product.cpp.objcopyPath.replace("objcopy", "nm"), args);
            cmd.stdoutFilePath = output.filePath;
            cmd.description = "generating " + output.fileName;
            return cmd;
        }
    }
}
