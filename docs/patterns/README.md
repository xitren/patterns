## Документация паттернов

Этот каталог содержит **отдельные подробные документы** по каждому паттерну/компоненту библиотеки.

- `transaction.md` — транзакция (RAII-rollback)
- `static-heap-and-allocator.md` — статическая куча и STL-аллокатор
- `lru-cache.md` — LRU cache с TTL
- `observer.md` — Observer (обычный) + статический/динамический варианты
- `observer-values.md` — Observer для “значений” (`comm::values`)
- `mediator.md` — Mediator (обмен сообщениями между модулями)
- `pipeline.md` — Pipeline stage / pool (потоковая обработка, bounded buffer)
- `argv-parser.md` — парсер параметров командной строки
- `endian-wrappers.md` — LSB/MSB-обёртки и байтовые утилиты (`data.hpp`)
- `fast-pimpl.md` — fast PIMPL (in-place storage)
- `packet.md` — packet serializer/deserializer (+ packet_accessor)
- `interval-event.md` — периодическое событие в отдельном потоке

