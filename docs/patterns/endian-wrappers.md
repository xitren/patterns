## LSB/MSB abstraction + byte utils (`xitren::func::data.hpp`)

### Что делает
Набор низкоуровневых утилит:
- `data<T>`: безопасная сериализация/десериализация trivially-copyable типов в массив байт.
- `lsb_t<T>` / `msb_t<T>`: обёртки, которые хранят целое число в порядке байт LSB/MSB независимо от endianness платформы.

### Когда применяется
- Работа с протоколами/пакетами, где задан порядок байт (LE/BE).
- Сериализация фиксированных POD-структур в байтовый поток.

### Пример применения

```cpp
using xitren::func::lsb_t;

lsb_t<std::uint32_t> le{0x11223344};
auto host = le.get(); // вернёт то же значение на любой платформе
```

```mermaid
flowchart LR
    A[T value] --> B[data<T>::serialize]
    B --> C[bytes[N]]
    C --> D[data<T>::deserialize]
    D --> A2[T value]
    A2 --> E{round-trip ok?}
    E -->|yes| OK[success]
    E -->|no| FAIL[bug/unsupported type]
```

### Что происходит внутри (подробно)
- `data<T>`:
  - ограничение `static_assert(std::is_trivially_copyable_v<T>)`.
  - `serialize` делает `memcpy` в `std::array<uint8_t, sizeof(T)>`.
  - `deserialize` делает `memcpy` обратно в `T`.
- `lsb_t/msb_t`:
  - при записи, если endianness платформы не совпадает с требуемым, выполняется `swap(...)`.
  - `get()` возвращает “host order” значение.

```mermaid
flowchart TD
    W[lsb_t<T> wrapper] --> S{native endian?}
    S -- little --> Store[store value as-is]
    S -- big --> Swap[byte-swap then store]
    Read[get()] --> S2{native endian?}
    S2 -- little --> Out[return stored]
    S2 -- big --> Out2[return swap(stored)]
```

### Как контролировать успешность операций
- **Сериализация**: round-trip `deserialize(serialize(x)) == x`.
- **Порядок байт**: тесты на известных константах/байтовых массивах.

