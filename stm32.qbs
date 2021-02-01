import qbs

Project {
    minimumQbsVersion: "1.9"

    property string device: "STM32F103C8T6"
    property bool cmsis_dsp: false
    property bool bootloader: false
    property bool use_stdio: true
    property int hwver: 0x0001
    property int swver: 0x0001
    property string hwver_str: ("0000" + hwver.toString(16)).slice(-4)
    property string swver_str: ("0000" + swver.toString(16)).slice(-4)

    references: [
        "cmsis",
        "core.qbs",
    ]

    CppApplication {
        name: project.name
        type: ["application", "hex", "size", "map"]
        Depends {name: "core"}
        Depends {name: "gcc-none"}
        Depends {name: "CMSIS"}

        files: [
            "main.c",
        ]

        Group {
            name: "Linker script"
            condition: project.use_stdio
            files: [
                "ld/0_STM32F103C8T6_FLASH.ld",
                "ld/1_main.ld",
                "ld/2_common.ld",
            ]
        }

        Group {
            name: "Linker script without heap"
            condition: !project.use_stdio
            files: [
                "ld/0_STM32F103C8T6_FLASH.ld",
                "ld/2_common_noheap.ld",
            ]
        }

        Group {     // Properties for the produced executable
            fileTagsFilter: ["application", "hex", "bin", "map"]
            qbs.install: true
        }
    }
}
