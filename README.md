# ParallelReduce

Задача проекта — реализовать собственный многопоточный `reduce` и сравнить его с алгоритмами стандартной библиотеки:
- `std::reduce` в последовательном режиме
- `std::reduce` в параллельном режиме

Основная идея проекта:
- написать собственный `my_reduce_seq`
- написать собственный `my_reduce_par`
- измерить время работы на одинаковых входных данных
- сравнить результаты и производительность со стандартной библиотекой

-----

# Описание

`reduce` — это свёртка диапазона значений в одно итоговое значение с помощью бинарной операции.

В данном проекте рассматривается сценарий суммирования чисел:
- `int64`
- `double`

Реализованы два режима собственной свёртки:

- `my_reduce_seq` — последовательный reduce
- `my_reduce_par` — параллельный reduce на нескольких потоках

Параллельная версия делит входной диапазон на блоки, считает частичные суммы в нескольких потоках и затем схлопывает их в финальный результат.

---

## Что сравнивается в benchmark

Во втором бинарнике сравниваются:

- `std_accumulate` (больше для проверки)
- `std_reduce_seq`
- `my_reduce_seq`
- `my_reduce_par`
- `std_reduce_par`

---

## Зависисмости
- компилятор с поддержкой C++17
- CMake
- Python 3
- Conan

## Записук проекта

```bash
git clone https://github.com/MaxGud10/ParallelReduce
cd ParallelReduce
```

Создадим виртуальную среду и установим Conan:
```bash
python3 -m venv .venv && source .venv/bin/activate && pip3 install conan
```

Установим зависимости проекта с помощью Conan:
```bash
conan install . -of third_party -s build_type=Release --build=missing
```

- `Основной режим`
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/reduce_run < ../tests/e2e/generated/test_100.dat
```

- `Debug`
```bash
conan install . -of third_party -s build_type=Debug --build=missing

cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/reduce_run < ../tests/e2e/generated/test_100.dat
```


### `reduce_run`
Основной режим запуска reduce на входных данных.

Поддерживает:
- `--mode seq|par`
- `--type int64|double`
- `--threads N`
- `--min-block-size N`

<details>
<summary> Подробнее что значат</summary>


| Аргумент | Что делает |
|---|---|
| `--mode seq` | Запускает последовательный reduce |
| `--mode par` | Запускает многопоточный reduce |
| `--type int64` | Обрабатывает целые числа |
| `--type double` | Обрабатывает числа с плавающей точкой |
| `--threads N` | Число потоков для режима `par` |
| `--min-block-size N` | Минимальный размер блока на один worker |


</details>



### `reduce_bench`
Режим benchmark.

Поддерживает:
- `--type int64|double`
- `--threads N`
- `--min-block-size N`

<details>
<summary> Подробнее что значат</summary>


| Аргумент | Что делает |
|---|---|
| `--type int64` | Бенчмарк для целых чисел |
| `--type double` | Бенчмарк для вещественных чисел |
| `--threads N` | Число потоков для `my_reduce_par` |
| `--min-block-size N` | Минимальный размер блока на worker |


</details>

### Формат входных данных

Оба бинарника читают вход в одном формате:

```text
N
x1 x2 x3 ... xN
```
где:
- N -количество чисел
- x1 x2 x3 ... xN - сами числа


---

## Тестирование

Проект содержит unit-тесты (GoogleTest) и end2end тесты.

1. Создайте тесты:
```bash
cmake --build build
```

2. Запустите тестовый двоичный файл:
```bash
cd build
ctest --output-on-failure
```

### Генерация e2e тестов
Для генерации больших входных данных используется скрипт:

```bash
tests/e2e/gen_large_e2e.py
```
Он создает:
- `name.dat` - вхоной файл
- `name.ans` - ожидаемый ответ

<details>
<summary> Примеры использваония</summary>
- `Маленький тест`
```bash
python3 tests/e2e/gen_large_e2e.py --count 100 --type int --output test_100
```

- `Большо1 тест`
```bash
python3 tests/e2e/gen_large_e2e.py --count 100 --type int --output test_100
```
где:
- `--count` - количество входных числе
- `--type` - тип самих числе
- `--output` - название выходных файлов
</details>