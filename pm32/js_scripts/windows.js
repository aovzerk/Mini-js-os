let windowX = 180
let windowY = 100
let windowDragging = 0
let windowDragX = 0
let windowDragY = 0
let windowMinimized = 0
let windowTitle = ""
let windowUsesTitle = 0
let previousPid = 0
let previousX = 0
let previousY = 0
let previousMinimized = 0
let previousTitle = ""
let previousUsesTitle = 0

const prepareNewWindow = () => {
    if (previousPid > 0) {
        myos.kill(previousPid)
    }
    previousPid = activePid
    previousX = windowX
    previousY = windowY
    previousMinimized = windowMinimized
    previousTitle = windowTitle
    previousUsesTitle = windowUsesTitle
    windowX = 220
    windowY = 130
    windowMinimized = 0
    windowDragging = 0
}

const drawOneWindow = (pid, x, y, title, minimized, taskX) => {
    if (pid > 0 && minimized == 0) {
        screen.fillRect(x + 8, y + 10, 800, 480, 463897)
        screen.fillRect(x, y, 800, 480, 1057070)
        screen.fillRect(x, y, 800, 2, 2152421)
        screen.fillRect(x, y + 2, 800, 40, 793383)
        screen.fillRect(x + 16, y + 13, 14, 14, 2152421)
        screen.drawText(x + 40, y + 12, "MYOS //")
        screen.drawText(x + 112, y + 12, title)
        screen.fillRect(x + 710, y + 7, 36, 28, 1322043)
        screen.fillRect(x + 752, y + 7, 40, 28, 2824226)
        screen.fillRect(x + 18, y + 52, 764, 397, 463897)
        screen.drawTerminal(pid, x + 26, y + 61, 748, 378)
    }
    if (pid > 0) {
        screen.fillRect(taskX, 678, 174, 32, 1390922)
        screen.fillRect(taskX, 678, 174, 2, 2152421)
        screen.drawText(taskX + 34, 686, title)
    }
}

const drawActiveWindow = () => {
    screen.setLayer(1)
    if (previousPid > 0 && previousUsesTitle == 1) {
        previousTitle = myos.getWindowTitle(previousPid)
    }
    if (activePid > 0 && windowUsesTitle == 1) {
        windowTitle = myos.getWindowTitle(activePid)
    }
    drawOneWindow(previousPid, previousX, previousY, previousTitle, previousMinimized, 242)
    if (activePid > 0 && windowMinimized == 0) {
        screen.fillRect(windowX + 8, windowY + 10, 800, 480, 463897)
        screen.fillRect(windowX, windowY, 800, 480, 1057070)
        screen.fillRect(windowX, windowY, 800, 2, 2152421)
        screen.fillRect(windowX, windowY + 2, 800, 40, 793383)
        screen.fillRect(windowX + 16, windowY + 13, 14, 14, 2152421)
        screen.fillRect(windowX + 20, windowY + 17, 6, 6, 463897)
        screen.drawText(windowX + 40, windowY + 12, "MYOS //")
        screen.drawText(windowX + 112, windowY + 12, windowTitle)
        screen.fillRect(windowX + 710, windowY + 7, 36, 28, 1322043)
        screen.fillRect(windowX + 721, windowY + 22, 14, 2, 9218744)
        screen.fillRect(windowX + 752, windowY + 7, 40, 28, 2824226)
        screen.fillRect(windowX + 764, windowY + 14, 3, 3, 16734324)
        screen.fillRect(windowX + 767, windowY + 17, 3, 3, 16734324)
        screen.fillRect(windowX + 770, windowY + 20, 3, 3, 16734324)
        screen.fillRect(windowX + 773, windowY + 17, 3, 3, 16734324)
        screen.fillRect(windowX + 776, windowY + 14, 3, 3, 16734324)
        screen.fillRect(windowX + 18, windowY + 52, 764, 397, 463897)
        screen.drawTerminal(activePid, windowX + 26, windowY + 61, 748, 378)
    }
    if (activePid > 0) {
        screen.fillRect(58, 678, 174, 32, 1390922)
        screen.fillRect(58, 678, 174, 2, 2152421)
        screen.drawText(92, 686, windowTitle)
    }
    screen.present()
}

const windowButton = (pressed) => {
    if (pressed == 1 && previousPid > 0 && mouseX >= 242 && mouseX < 416 && mouseY >= 678 && mouseY < 710) {
        const swapPid = activePid
        const swapX = windowX
        const swapY = windowY
        const swapMinimized = windowMinimized
        const swapTitle = windowTitle
        const swapUsesTitle = windowUsesTitle
        activePid = previousPid
        windowX = previousX
        windowY = previousY
        windowMinimized = 0
        windowTitle = previousTitle
        windowUsesTitle = previousUsesTitle
        previousPid = swapPid
        previousX = swapX
        previousY = swapY
        previousMinimized = swapMinimized
        previousTitle = swapTitle
        previousUsesTitle = swapUsesTitle
        screen.setLayer(1)
        screen.focus(activePid)
        drawActiveWindow()
    }
    if (activePid > 0 && pressed == 1) {
        if (windowMinimized == 1 && mouseX >= 58 && mouseX < 232 && mouseY >= 678 && mouseY < 710) {
            windowMinimized = 0
            screen.setLayer(1)
            screen.focus(activePid)
            drawActiveWindow()
        }
        if (windowMinimized == 0 && mouseX >= windowX + 752 && mouseX < windowX + 792 && mouseY >= windowY + 7 && mouseY < windowY + 35) {
            myos.kill(activePid)
            activePid = 0
            windowDragging = 0
            screen.setLayer(1)
            screen.focus(0)
            drawActiveWindow()
        }
        if (windowMinimized == 0 && mouseX >= windowX + 710 && mouseX < windowX + 746 && mouseY >= windowY + 7 && mouseY < windowY + 35) {
            windowMinimized = 1
            windowDragging = 0
            screen.setLayer(1)
            screen.focus(0)
            drawActiveWindow()
        }
        if (windowMinimized == 0 && mouseX >= windowX && mouseX < windowX + 700 && mouseY >= windowY && mouseY < windowY + 42) {
            windowDragging = 1
            windowDragX = mouseX - windowX
            windowDragY = mouseY - windowY
        }
    }
    if (pressed == 0) {
        windowDragging = 0
    }
}

const moveWindow = () => {
    if (windowDragging == 1 && windowMinimized == 0) {
        windowX = mouseX - windowDragX
        windowY = mouseY - windowDragY
        if (windowX < 0) {
            windowX = 0
        }
        if (windowY < 0) {
            windowY = 0
        }
        if (windowX > 480) {
            windowX = 480
        }
        if (windowY > 188) {
            windowY = 188
        }
        drawActiveWindow()
    }
}
