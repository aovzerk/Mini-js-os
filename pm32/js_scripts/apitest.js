require("myos")

console.log("MyOS JavaScript API test")
let writeResult = myos.writeFile("APIOUT.TXT", "Written from JavaScript")
console.log(writeResult)

let fileBytes = myos.readFile("APIOUT.TXT")
console.log(fileBytes)

let files = myos.listFiles()
console.log(files)

let pid = myos.getPid()
let appType = myos.getAppType()
console.log(pid)
console.log(appType)
myos.yield()

// JS.BIN is a console application, so kernel policy returns -1 here.
let windowResult = myos.createWindow()
console.log(windowResult)

console.log("API test complete")
