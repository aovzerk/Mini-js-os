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

Функция принимает до восьми параметров: числа, строки, объекты и массивы.
Каждый вызов создаёт локальный scope; структурированные аргументы глубоко
копируются, а локальные heap-значения освобождаются при возврате. Стрелочная
функция сейчас объявляется в переменной и использует блочное тело. Таймер принимает имя
функции и задержку в миллисекундах и возвращает числовой идентификатор.
`clearTimeout(id)` и `clearInterval(id)` отменяют соответствующую запись;
повторная отмена или неизвестный идентификатор безопасно игнорируются.
`setInterval` сохраняет JS event loop активным, пока существуют интервалы;
callbacks запускаются event loop по показаниям монотонных часов. Планировщик
может вытеснить JS-код в любой момент независимо от `sys_yield`.

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

Поддерживаются обычные массивы до 32 элементов, `length`, индексирование,
объекты внутри массивов и рекурсивный вывод структур:

```js
const windows = [{x: 10, title: "ONE"}, {x: 20, title: "TWO"}]
console.log(windows.length)
console.log(windows[0].x)
console.log(windows)
```

Массив `ProcessInfo`, возвращаемый системным модулем, пока остаётся отдельным
встроенным типом runtime. Стандартные browser/Node.js API отсутствуют.

Строковые литералы поддерживаются в объявлении и повторном присваивании:
`let title = "TERMINAL"`, `title = "PROCESS MONITOR"`.

Пользовательский JS-файл можно подключить отдельным оператором
`require("module.js")`. Перед разбором runtime объединяет модуль и основной
скрипт в одну программу, поэтому объявленные модулем функции доступны основному
файлу и таймерам. `require("myos")` по-прежнему возвращает системный модуль.

Поддерживаются простые объектные литералы с именованными числовыми и строковыми
свойствами и чтение свойств через точку:

```js
const value = { number: 100, text: "asd" }
console.log(value.number)
console.log(value.text)
console.log(value) // {number: 100, text: "asd"}
```

Объекты, вложенные объекты, массивы и их строки размещаются в процессном heap
и освобождаются рекурсивно. Поддерживается изменение и добавление свойств
верхнего уровня. Методы, прототипы и изменение свойства по вложенному пути пока
не реализованы.

## Системный модуль

```js
const myos = require("myos")
```

Основные методы:

- `readFile`, `writeFile`, `listFiles`;
- `exec`, `run`, `spawn`, `spawnGui`, `exit`, `yield`, `idle`;
- `getPid`, `getAppType`, `millis`, `readKey`, `sendKey`, `terminalRead`, `kill`;
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

GUI-runtime не создаёт окно автоматически: обычное приложение явно вызывает
`createWindow()`, а headless-компонент может использовать только `createScreen()`.

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

## Native-графика и события

GUI-скрипт может создать retained-слой композитора:

```js
const screen = myos.createScreen()
screen.setLayer(1)
screen.fillRect(20, 20, 200, 40, 2136037)
screen.drawText(32, 30, "JS GUI")
screen.present()
```

Команды передаются C-композитору через kernel IPC. `present()` фиксирует список
команд, поэтому слой повторно накладывается после обычных redraw рабочего
стола. Слой `0` рисуется под окнами, слой `1` — поверх них и используется по
умолчанию, слой `2` зарезервирован для курсора и прочего верхнего UI. Retained-
кадры изолированы по PID, поэтому разные JS-приложения не затирают друг друга.
`drawText` принимает литерал или строковую переменную. Доступны
`setLayer`, `fillRect`, `drawText`, `drawCursor` и `present`; драйвер
режима, framebuffer, шрифт и копирование backbuffer остаются в C.
`drawTerminal(pid, x, y, width, height)` растеризует постоянный terminal
snapshot процесса внутри прямоугольника, геометрию которого задаёт JS.
`drawImage("SHELLICO.PNG", x, y)` выводит PNG-ресурс, декодированный native
backend; выбор позиции и включение изображения в кадр остаются в JS.

`myos.pollEvent()` всегда возвращает объект с полями
`type`, `x`, `y`, `value`. Тип `1` обозначает клавишу. Тип `2` — перемещение
мыши; его `value` сообщает, удерживается ли левая кнопка, чтобы drag мог
безопасно завершиться даже при потерянном release. Тип `3` — отдельный фронт
кнопки со значением `1` для press и `0` для release. Пустая очередь обозначается
объектом с `type: 0`. Значение кнопки `2` обозначает отменённый цикл, например
release, смешанный с движением; такой цикл не считается кликом.

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

JS-код вытесняется системным таймером наравне с C-программами. Вызов
`myos.yield()` доступен, но не нужен для корректной многозадачности; runtime
использует его в пустом event loop только для добровольной отдачи CPU.
