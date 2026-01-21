## Command-line parameter parser (`xitren::func::argv_parser`)

### Что делает
Парсер аргументов командной строки (`argc/argv`) в структуру `Opts` по таблице соответствий `"--flag" -> member-pointer`.

### Когда применяется
- Нужны простые флаги/параметры без внешних зависимостей.
- Хочется декларативно сопоставить ключи с полями структуры.

### Пример применения

```cpp
struct options {
  std::string port{};
  int baud{};
  bool help{};
};

auto parser = xitren::func::argv_parser<options>::instance({
  {"--port", &options::port},
  {"--baud", &options::baud},
  {"--help", &options::help},
});

options opt = parser->parse(argc, argv);
```

```mermaid
flowchart TD
    A[parse(argc, argv)] --> B[for idx in argv]
    B --> C{argv[idx] is known key?}
    C -- no --> B
    C -- yes --> D{target member type}
    D -- bool --> E[set member=true]
    D -- int --> F[from_chars -> int]
    D -- double --> G[from_chars -> double\nfallback stod]
    D -- string --> H[copy to std::string]
    E --> B
    F --> B
    G --> B
    H --> B
```

### Что происходит внутри (подробно)
- `instance(...)` создаёт объект и сохраняет таблицу аргументов.
- `parse(argc, argv)`:
  - проходит один раз по массиву аргументов,
  - если текущий токен совпал с известным ключом:
    - для `bool` — выставляет `true`,
    - для `int` — парсит `std::from_chars`,
    - для `double` — пытается `from_chars`, при неудаче fallback на `std::stod`,
    - для `std::string` — копирует `string_view` в `std::string`.

### Как контролировать успешность операций
- **Признак успеха**: поля `Opts` заполнены ожидаемыми значениями.
- **Проверка**: сравнение полей с ожидаемыми (как в тестах проекта).
- **Ошибки/невалидные значения**:
  - сейчас не выбрасывается явная ошибка, а значение просто не меняется, если парсинг не удался.
  - если нужен строгий режим — можно расширить API (например, возвращать `std::optional<Opts>` + error list).

