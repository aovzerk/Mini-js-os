const myos = require("myos")
const window = myos.createWindow()
window.setTitle("PROCESS MONITOR")

while (true) {
    const processes = myos.listProcesses()

    window.clear()
    window.write("MYOS PROCESS MONITOR\n")
    window.write("Active process table\n\n")
    window.writeCell("PID", 4)
    window.writeCell("NAME", 10)
    window.writeCell("STATE", 7)
    window.writeCell("TYPE", 9)
    window.writeCell("PARENT", 8)
    window.write("TERM\n")

    for (let i = 0; i < processes.length; i++) {
        window.writeCell(processes[i].pid, 4)
        window.writeCell(processes[i].name, 10)
        window.writeCell(processes[i].state, 7)
        window.writeCell(processes[i].type, 9)
        window.writeCell(processes[i].parentPid, 8)
        window.writeCell(processes[i].terminalPid, 4)
        window.write("\n")
    }

    for (let delay = 0; delay < 200; delay++) {
        myos.yield()
    }
}
