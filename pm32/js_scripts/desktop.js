const myos = require("myos")
require("deskapps.js")
require("windows.js")
const screen = myos.createScreen()
let mouseX = 640
let mouseY = 360
let activePid = 0
let desktopPressPending = 0
let desktopPressX = 0
let desktopPressY = 0
let lastCursorDraw = 0
let cursorDirty = 1

screen.setLayer(0)
screen.fillRect(0, 0, 1280, 668, 463897)
screen.fillRect(0, 0, 5, 668, 1390922)

screen.fillRect(22, 22, 76, 92, 729385)
screen.fillRect(22, 22, 76, 2, 2152421)
screen.fillRect(28, 28, 64, 64, 1059643)
screen.drawImage("SHELLICO.PNG", 36, 36)
screen.drawText(28, 96, "TERMINAL")

screen.fillRect(110, 22, 76, 92, 729385)
screen.fillRect(110, 22, 76, 2, 10906879)
screen.fillRect(116, 28, 64, 64, 1059643)
screen.fillRect(130, 38, 36, 44, 14477551)
screen.fillRect(136, 47, 24, 2, 4612723)
screen.fillRect(136, 56, 24, 2, 4612723)
screen.fillRect(136, 65, 18, 2, 4612723)
screen.drawText(124, 96, "EDITOR")

screen.fillRect(198, 22, 76, 92, 729385)
screen.fillRect(198, 22, 76, 2, 5625994)
screen.fillRect(204, 28, 64, 64, 1059643)
screen.fillRect(214, 40, 44, 32, 1455942)
screen.fillRect(220, 58, 7, 8, 5625994)
screen.fillRect(231, 50, 7, 16, 5625994)
screen.fillRect(242, 44, 7, 22, 5625994)
screen.drawText(204, 96, "MONITOR")

screen.fillRect(0, 668, 1280, 52, 595746)
screen.fillRect(0, 668, 1280, 1, 1785940)
screen.fillRect(12, 677, 34, 32, 2824226)
screen.fillRect(12, 677, 34, 2, 16734324)
screen.fillRect(27, 684, 4, 11, 16734324)
screen.fillRect(21, 691, 3, 10, 16734324)
screen.fillRect(34, 691, 3, 10, 16734324)
screen.fillRect(24, 700, 10, 3, 16734324)
screen.fillRect(1130, 682, 80, 24, 1059643)
screen.drawText(1142, 686, "MYOS 32")

screen.present()

const drawCursor = () => {
    const now = myos.millis()
    if (cursorDirty == 0 || now - lastCursorDraw < 16) {
        return
    }
    lastCursorDraw = now
    cursorDirty = 0
    screen.setLayer(2)
    screen.drawCursor(mouseX, mouseY)
    screen.present()
}

const updateDesktop = () => {
    const event = myos.pollEvent()
    if (event.type > 0) {
        if (event.type == 2) {
            mouseX = event.x
            mouseY = event.y
            cursorDirty = 1
            if (event.value == 0) {
                windowDragging = 0
            }
            moveWindow()
            drawCursor()
        }
        if (event.type == 3) {
            mouseX = event.x
            mouseY = event.y
            cursorDirty = 1
            if (event.value == 1) {
                desktopPressPending = 1
                desktopPressX = mouseX
                desktopPressY = mouseY
                if (activePid > 0) {
                    windowButton(1)
                }
            }
            if (event.value == 0) {
                if (desktopPressPending == 1 && mouseX >= desktopPressX - 4 && mouseX <= desktopPressX + 4 && mouseY >= desktopPressY - 4 && mouseY <= desktopPressY + 4) {
                    if (desktopPressY >= 22 && desktopPressY < 114 && desktopPressX >= 22 && desktopPressX < 98) {
                        launchTerminal()
                    }
                    if (desktopPressY >= 22 && desktopPressY < 114 && desktopPressX >= 110 && desktopPressX < 186) {
                        launchEditor()
                    }
                    if (desktopPressY >= 22 && desktopPressY < 114 && desktopPressX >= 198 && desktopPressX < 274) {
                        launchMonitor()
                    }
                    if (desktopPressY >= 677 && desktopPressY < 709 && desktopPressX >= 12 && desktopPressX < 46) {
                        shutdownDesktop()
                    }
                    drawActiveWindow()
                }
                desktopPressPending = 0
                windowButton(0)
            }
            if (event.value == 2) {
                desktopPressPending = 0
                windowButton(0)
            }
            drawCursor()
        }
        if (event.type == 1) {
            if (activePid > 0 && windowMinimized == 0) {
                myos.sendKey(activePid, event.value)
            }
        }
    }
    if (cursorDirty == 1) {
        drawCursor()
    }
}

drawCursor()
drawActiveWindow()
setInterval(updateDesktop, 5)
setInterval(drawActiveWindow, 250)
