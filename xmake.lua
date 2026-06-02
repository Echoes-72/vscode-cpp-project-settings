set_config("buildir", ".vscode/build")
add_rules("mode.debug", "mode.release")

target("App")
    set_toolset("clang")
    add_rules("qt.widgetapp")
    add_files("src/*.cpp")
    add_files("UI/widget.ui")
    add_files("src/widget.h")