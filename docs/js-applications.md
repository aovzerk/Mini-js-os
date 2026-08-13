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
`++`, `--`, пользовательские функции, `return`, блочные стрелочные функции,
`setTimeout`, `setInterval`, `clearTimeout`, `clearInterval` и `console.log()`.

```js
function add(left, right) {
    return left + right
}

const delayed = () => {
    console.log("done")
}

console.log(add(20, 22))
const timeoutId = setTimeout(delayed, 100)
clearTimeout(timeoutId)
```

Функция принимает до четырёх числовых параметров. Стрелочная функция сейчас
объявляется в переменной и использует блочное тело. Таймер принимает имя
функции и задержку в миллисекундах и возвращает числовой идентификатор.
`clearTimeout(id)` и `clearInterval(id)` отменяют соответствующую запись;
повторная отмена или неизвестный идентификатор безопасно игнорируются.
`setInterval` сохраняет JS event loop активным, пока существуют интервалы;
callbacks выполняются кооперативно между вызовами `sys_yield`.

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

Обычные массивы и стандартные browser/Node.js API пока отсутствуют. Массив
`ProcessInfo`, возвращаемый системным модулем,
является встроенным типом runtime.

Поддерживаются простые объектные литералы с именованными числовыми и строковыми
свойствами и чтение свойств через точку:

```js
const value = { number: 100, text: "asd" }
console.log(value.number)
console.log(value.text)
console.log(value) // {number: 100, text: "asd"}
```

Объекты и их строки размещаются в процессном heap. Вложенные объекты, методы,
прототипы и изменение отдельных свойств пока не реализованы.

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
возвращают динамические строки размером до 4096 байт, а остальные системные
методы — числовые коды или значения.
Встроенные методы `myos` ничего не выводят: явный вывод выполняется только
через `console.log()`.

`writeFile(name, value)` принимает как строковый литерал, так и строковую
переменную, включая результат `readFile()`.

Исходный текст скрипта и строки, возвращённые системным модулем, размещаются в
процессном heap. При перезаписи строковой переменной и завершении runtime память
освобождается и обнуляется ядром.

## Окно

```js
const myos = require("myos")
const window = myos.createWindow()
window.setTitle("MY APPLICATION")
window.clear()
window.write("Hello\n")
window.wait()
```

Доступны `setTitle(text)`, `clear()`, `beginUpdate()`, `endUpdate()`,
`write(value)`, `writeCell(value, width)` и `wait()`. `writeCell` дополняет значение пробелами
и позволяет приложению самостоятельно формировать таблицы.

`beginUpdate()` очищает скрытое содержимое окна и начинает пакетное обновление.
До `endUpdate()` GUI принимает все порции текста, но не перерисовывает окно.
`endUpdate()` публикует готовый кадр одной перерисовкой, что устраняет мерцание
периодически обновляемых таблиц.

## Процессы

`myos.listProcesses()` возвращает массив встроенных объектов. Поле `parentPid`
позволяет строить дерево процессов:

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

- `JSTEST.JS` проверяет язык, комментарии, функции, объекты и создание/отмену
  timeout/interval;
- `APITEST.JS` проверяет системный модуль;
- `MONITOR.JS` — автообновляемое GUI-приложение, которое запускается как
  `jsgui monitor.js`, само формирует дерево по `parentPid` и публикует готовый
  кадр через `beginUpdate()`/`endUpdate()`.

После изменений runtime нужно отдельно проверять консольный `JS.BIN` и путь
GUI → `JSGUI.BIN` → `MONITOR.JS` на отсутствие `check_exception` и
`Triple fault`.
