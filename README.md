# fast-kv — высокопроизводительное in-memory key-value хранилище на C++

`fast-kv` — это учебный pet-project, вдохновленный архитектурой Redis.  
Проект реализует собственный текстовый протокол, epoll-сервер, пул потоков и in-memory хранилище с поддержкой базовых операций.

Цель проекта — показать навыки разработки высоконагруженных сетевых сервисов на C++, работу с многопоточностью, неблокирующим IO и низкоуровневыми механизмами Linux.

---

##  Возможности

- Собственный текстовый протокол (`SET`, `GET`, `DEL`, `INCR`, `EXISTS`, `COUNT`, `KEYS`, `CLEAR`, `STATS`).
- Полностью **in-memory** хранилище на основе `unordered_map`.
- **Пул потоков** (thread pool) для выполнения команд.
- **epoll + non-blocking sockets** — один процесс обслуживает множество клиентов.
- Буферизация запроса на уровне соединения.
- Логирование в файл (`fast-kv.log`) с авто-ротацией по размеру.
- Мониторинг состояния (`STATS`):  
  - активные соединения  
  - всего соединений  
  - количество команд  
  - количество ошибок  

---

## Архитектура

Проект построен по модульной архитектуре:

server.cpp
├── epoll-мультиплексирование событий
, non-blocking сокеты
, буфер входящих данных
, передача полной строки в thread pool
, отправка ответа клиенту

thread_pool.cpp
├── фиксированное количество потоков
, очередь задач

kv_store.cpp
├── реализация всех команд протокола
, потокобезопасный доступ (mutex)
, in-memory key-value map

parser.cpp
└── разбор строки на токены

stats.cpp
└── метрики (atomic counters)

logger.cpp
├── логирование в файл
, автоматическая ротация при >1MB

yaml
Copy code

---
##  Запуск

### 1. Сборка

```
mkdir build
cd build
cmake ..
make
2. Запуск сервера
Copy code
./server
По умолчанию сервер слушает порт 8080.
```
 Примеры использования
Открой второй терминал и подключись:

```
Copy code
nc localhost 8080
Примеры команд:
sql
Copy code
SET a 10
OK

GET a
VALUE 10

INCR a
VALUE 11

EXISTS a
1

COUNT
1

KEYS
KEYS a

DEL a
DELETED

STATS
STATS connections_now=1 connections_total=3 commands_total=15 errors_total=0
Каждая команда завершается переносом строки \n.
```
Поддерживаемые команды
```
Команда	Описание
SET key value	установить значение
GET key	получить значение
DEL key	удалить ключ
EXISTS key	1 если ключ есть, иначе 0
INCR key	увеличить значение на 1 (числовое)
COUNT	количество ключей
KEYS	вывести список ключей
CLEAR	очистить хранилище
STATS	вернуть метрики системы
```
Структура проекта
```
src/
 ├── server.cpp
 ├── kv_store.cpp
 ├── kv_store.h
 ├── parser.cpp
 ├── parser.h
 ├── thread_pool.cpp
 ├── thread_pool.h
 ├── logger.cpp
 ├── logger.h
 ├── stats.cpp
 └── stats.h
CMakeLists.txt
README.md

