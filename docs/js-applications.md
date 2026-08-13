# JavaScript-приложения MyOS

`JS.BIN` выполняет консольный `.JS`-файл, `JSGUI.BIN` — GUI-скрипт с
`AppType = 1`.

```text
> js jstest.js
```

Без имени файла runtime выводит `usage: js FILE.JS`.

## Поддерживаемый язык

Реализованы `let`, `var`, `const`, целые числа, boolean, строки, присваивания,
арифметика, сравнения, `&&`, `||`, `!`, `if/else`, `while`, классический `for`,
`++`, `--` и `console.log()`.

Каждая строка является отдельным оператором. Открывающая фигурная скобка
находится в строке управляющего оператора, закрывающая — отдельно.

```js
let sum = 0
for (let i = 0; i < 5; i++) {
    sum = sum + i
}
console.log(sum)
```

Поддерживаются `//` и многострочные `/* ... */` комментарии. Маркеры внутри
строк не считаются комментариями. Незакрытый блок выдаёт
`JS error: unterminated block comment`.

Пользовательские функции, произвольные объекты, обычные массивы, heap и
стандартные browser/Node.js API пока отсутствуют. Массив `ProcessInfo`,
возвращаемый системным модулем, является встроенным типом runtime.

## Системный модуль

```js
const myos = require("myos")
```

Основные методы:

- `readFile`, `writeFile`, `listFiles`;
- `exec`, `run`, `spawn`, `spawnGui`, `exit`, `yield`, `idle`;
- `getPid`, `getAppType`, `readKey`, `sendKey`, `terminalRead`, `kill`;
- `createWindow`, `nextWindow`, `listProcesses`, `poweroff`.

Файловые имена используют FAT 8.3. `readFile`, `listFiles` и `terminalRead`
возвращают строки, а остальные системные методы — числовые коды или значения.
Встроенные методы `myos` ничего не выводят: явный вывод выполняется только
через `console.log()`.

## Окно

```js
const myos = require("myos")
const window = myos.createWindow()
window.setTitle("MY APPLICATION")
window.clear()
window.write("Hello\n")
window.wait()
```

Доступны `setTitle(text)`, `clear()`, `write(value)`,
`writeCell(value, width)` и `wait()`. `writeCell` дополняет значение пробелами
и позволяет приложению самостоятельно формировать таблицы.

## Процессы

`myos.listProcesses()` возвращает массив встроенных объектов:

```js
const processes = myos.listProcesses()
for (let i = 0; i < processes.length; i++) {
    window.writeCell(processes[i].pid, 4)
    window.writeCell(processes[i].name, 10)
    window.writeCell(processes[i].state, 7)
    window.writeCell(processes[i].type, 9)
    window.write("\n")
}
```

Поля процесса: `pid`, `name`, `active`, `state`, `type`, `appType`,
`parentPid`, `terminalPid`.

## Скрипты в образе

- `JSTEST.JS` проверяет язык и комментарии;
- `APITEST.JS` проверяет системный модуль;
- `MONITOR.JS` — автообновляемое GUI-приложение, которое запускается как
  `jsgui monitor.js` и само формирует таблицу процессов.

После изменений runtime нужно отдельно проверять консольный `JS.BIN` и путь
GUI → `JSGUI.BIN` → `MONITOR.JS` на отсутствие `check_exception` и
`Triple fault`.
