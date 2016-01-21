import qbs

Project {
    qbsSearchPaths: [ "/home/thomas/projects/own/stm32" ]
    references: [ "../wos/wos.qbs" ]

    CppApplication {
        property string wosPath: "../wos"
        type: "application" // To suppress bundle generation on Mac
        consoleApplication: true
        files: [
            "StmSystem.cpp",
            "StmSystem.h",
            "main.cpp",
        ]

        cpp.warningLevel: "all"
        cpp.cxxLanguageVersion: "c++11"
        cpp.debugInformation: true
        cpp.optimization: "debug"
        cpp.linkerScripts: [ "stm32f407vg.ld" ]
        cpp.positionIndependentCode: false

        Depends { name: "wos" }
//        cpp.includePaths: [".", wos.sourceDirectory]
        cpp.libraryPaths: ["../build-wos-stm32-Debug/qtc_stm32_4f87d225-debug/install-root/", "../build-wos-stm32-Debug/qtc_stm32_4f87d225-debug/install-root/"]
        cpp.staticLibraries: [ "wos" ]

        cpp.commonCompilerFlags: [
            "-mcpu=cortex-m4",
            "-mthumb",
            "-mfpu=fpv4-sp-d16",
            "-mfloat-abi=softfp",
            "-fno-rtti",
            "-fno-exceptions"
        ]
        cpp.linkerFlags: [
            "-mcpu=cortex-m4",
            "-mthumb",
            "-mfpu=fpv4-sp-d16",
            "-mfloat-abi=softfp",
            "-nostartfiles",
        ]

        Group {     // Properties for the produced executable
            fileTagsFilter: product.type
            qbs.install: true
        }
    }


}