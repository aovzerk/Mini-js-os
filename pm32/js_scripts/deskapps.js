const launchTerminal = () => {
    prepareNewWindow()
    activePid = myos.spawn("shell")
    windowTitle = "TERMINAL"
    windowUsesTitle = 0
    windowMinimized = 0
    screen.setLayer(1)
    screen.focus(activePid)
}

const launchEditor = () => {
    prepareNewWindow()
    activePid = myos.spawnGui("editor")
    windowUsesTitle = 1
    windowMinimized = 0
    screen.setLayer(1)
    screen.focus(activePid)
}

const launchMonitor = () => {
    prepareNewWindow()
    activePid = myos.spawnGui("jsgui monitor.js")
    windowUsesTitle = 1
    windowMinimized = 0
    screen.setLayer(1)
    screen.focus(activePid)
}

const shutdownDesktop = () => {
    myos.poweroff()
}
