const myos = require("myos")
const window = myos.createWindow()
window.setTitle("PROCESS MONITOR")


const writeProcessInfo = () => {
    const processes = myos.listProcesses()

    window.beginUpdate()
    window.write("MYOS PROCESS MONITOR\n")
    window.write("Active process tree\n\n")
    window.writeCell("TREE", 10)
    window.writeCell("NAME", 10)
    window.writeCell("STATE", 7)
    window.writeCell("TYPE", 9)
    window.writeCell("PARENT", 8)
    window.write("TERM\n")

    for (let i = 0; i < processes.length; i++) {
        if (processes[i].pid == processes[i].parentPid) {
            window.write("* ")
            window.writeCell(processes[i].pid, 8)
            window.writeCell(processes[i].name, 10)
            window.writeCell(processes[i].state, 7)
            window.writeCell(processes[i].type, 9)
            window.writeCell(processes[i].parentPid, 8)
            window.writeCell(processes[i].terminalPid, 4)
            window.write("\n")

            for (let j = 0; j < processes.length; j++) {
                if (processes[j].pid != processes[i].pid) {
                    if (processes[j].parentPid == processes[i].pid) {
                        window.write("  |- ")
                        window.writeCell(processes[j].pid, 5)
                        window.writeCell(processes[j].name, 10)
                        window.writeCell(processes[j].state, 7)
                        window.writeCell(processes[j].type, 9)
                        window.writeCell(processes[j].parentPid, 8)
                        window.writeCell(processes[j].terminalPid, 4)
                        window.write("\n")

                        for (let k = 0; k < processes.length; k++) {
                            if (processes[k].pid != processes[j].pid) {
                                if (processes[k].parentPid == processes[j].pid) {
                                    window.write("     |- ")
                                    window.writeCell(processes[k].pid, 2)
                                    window.writeCell(processes[k].name, 10)
                                    window.writeCell(processes[k].state, 7)
                                    window.writeCell(processes[k].type, 9)
                                    window.writeCell(processes[k].parentPid, 8)
                                    window.writeCell(processes[k].terminalPid, 4)
                                    window.write("\n")
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    window.endUpdate()
}
setInterval(writeProcessInfo, 500)
