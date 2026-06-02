set_config("buildir", ".vscode/build")
add_rules("mode.debug", "mode.release")

target("Temp")
    add_rules("qt.widgetapp")
    add_files("src/*.cpp")
    add_files("src/widget.ui")
    -- add files with Q_OBJECT meta (only for qt.moc)
    add_files("src/widget.h")