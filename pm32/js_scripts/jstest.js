let sum = 0

// A line comment may contain tokens: while (true) { }
/* Block comments may span
   multiple source lines. */
let commentTest = 2 /* inline block comment */ + 3 // trailing comment

for (let i = 0; i < 5; i++) {
    sum = sum + i
}

if (sum == 10) {
    console.log(sum)
}
else {
    console.log(0)
}

while (sum < 12) {
    sum++
}
const a = {a: 100, b:"asd"}
console.log(a.a)
console.log(a.b)
console.log(a);
a.a = 200
a.c = "new"
console.log(a)
const windows = [{x: 10, title: "ONE"}, {x: 20, title: "TWO"}]
console.log(windows)
console.log(windows.length)
console.log(windows[0].x)
const theme = {colors: {background: "navy", text: "white"}}
console.log(theme)

function inspectWindow(config, label) {
    let localValue = 77
    config.x = 99
    console.log(label)
    console.log(config)
    console.log(localValue)
}

inspectWindow(windows[0], "window copy")
console.log(windows[0].x)
console.log(sum)
console.log(commentTest)
console.log("// and /* stay inside strings */")

function add(left, right) {
    return left + right
}

const delayed = () => {
    console.log("timeout")
}

console.log(add(20, 22))
setTimeout(delayed, 5000)

const canceledTimeout = () => {
    console.log("ERROR: canceled timeout fired")
}

let canceledTimeoutId = setTimeout(canceledTimeout, 200)
clearTimeout(canceledTimeoutId)

let intervalCount = 0
const intervalCallback = () => {
    intervalCount++
    console.log(intervalCount)
    if (intervalCount == 3) {
        clearInterval(intervalId)
    }
}

let intervalId = setInterval(intervalCallback, 50)
