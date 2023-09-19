namespace amu {

void Debugger::
start() {
    setlocale(LC_ALL, "en_US.UTF-8");
    notcurses_options opt{
        .loglevel = NCLOGLEVEL_WARNING,
        .flags = NCOPTION_CLI_MODE,
    };
    auto nc = notcurses_init(&opt, stdout);
    if(!nc) {
        util::println("notcurses failed");
    } else {
        util::println("notcurses loaded");
    }

    notcurses_stop(nc);
}

} // namespace amu