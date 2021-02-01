import qbs

StaticLibrary {
    name: "core"
    Depends {name: "CMSIS"}

    cpp.defines: {
        var flags = ["HWVER=" + project.hwver, "SWVER=" + project.swver];
        if (project.bootloader)
            flags.push("BOOTLOADER");
        if (project.use_stdio)
            flags.push("USE_STDIO");
        return flags;
    }

    cpp.commonCompilerFlags: [
        "-Wno-unused-parameter",
        "-Wno-unused-function",
        "-Wno-unused-variable",
    ]
    cpp.staticLibraries: ["m"]
    cpp.includePaths: ["common", ".", "../assets/fonts/dist"]

    Properties {
        condition: qbs.buildVariant == "debug"
        cpp.defines: outer.concat(["DEBUG=5"])
        cpp.optimization: "none"
    }

    Properties {
        condition: qbs.buildVariant == "release"
        cpp.defines: outer.concat(["DEBUG=1"])
        cpp.optimization: "fast"
    }

    Properties {
        condition: project.bootloader
        cpp.defines: outer.concat(["BOOTLOADER"])
    }

    Export {
        Depends {name: "cpp"}
        Parameters {cpp.linkWholeArchive: true}
        cpp.includePaths: product.cpp.includePaths
        cpp.defines: product.cpp.defines
        cpp.driverFlags: product.cpp.driverFlags
        cpp.optimization: product.cpp.optimization
    }

    Group {
        name: "Memory test"
        condition: false
        cpp.optimization: "none"
        cpp.commonCompilerFlags: ["-O3"]
        files: ["system/mem_test.c"]
    }

    Group {
        name: "Syscall"
        condition: project.use_stdio
        files: ["system/syscall.c"]
    }

    Group {
        name: "Critical"
        cpp.optimization: "none"
        cpp.commonCompilerFlags: ["-O3"]
        files: [
            "system/clocks.h",
            "system/clocks.c",
            "system/systick.h",
            "system/systick.c",
        ]
    }

    files: [
        "common/common.c",
        "common/common.h",
        "common/escape.h",
        "common/list.h",
        "common/macros.h",
        "common/debug.h",
        "common/device.h",
        //"peripheral/button.c",
        //"peripheral/button.h",
        "peripheral/gpio.c",
        "peripheral/gpio.h",
        "peripheral/spi_master.c",
        "peripheral/spi_master.h",
        "devices/nfc_defs.h",
        "devices/pn5180.c",
        "devices/pn5180.h",
        "devices/pn5180_defs.h",
        "devices/pn5180_update.c",
        "devices/PN5180Firmware_4.1.h",
        "system/system.c",
        "system/system.h",
        "system/system.s",
        "system/irq.h",
        "system/dma.txt",
    ]
}
