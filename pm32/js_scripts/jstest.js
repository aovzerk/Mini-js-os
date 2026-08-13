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

console.log(sum)
console.log(commentTest)
console.log("// and /* stay inside strings */")
