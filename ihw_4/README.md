# ИДЗ 4 ОС "Сетевые взаимодействия с применением транспортного протокола UDP"

## Оробцов Вячеслав Алексеевич БПИ214

### Условие задачи (Вариант 36)

Задача о сельской библиотеке.

В библиотеке имеется N книг, каждая из книг в одном экземпляре. M читателей регулярно заглядывают в библиотеку, каждый из них выбирает для чтения одну книгу и читает ее некоторое количество дней. Если желаемой книги нет, то читатель дожидается от библиотекаря информации об ее появлении и приходит в библиотеку, чтобы специально забрать ее. Возможна ситуация, когда несколько читателей конкурируют из-за этой популярной книги. Создать приложение, моделирующее заданный процесс. Библиотекарь и читатели должны быть представлены в виде отдельных процессов.

## Сценарий решаемой задачи (4-5 баллов)

Библиотекарь - отдельная программа на C с названием `librarian`, которую следует запускать со следующими аргументами командной строки:

`./librarian N PORT1 PORT2`, где N - натуральное число, означающее количество книг в библиотеке. Процесс библиотекаря создаст массив для N чисел который далее будет называться библиотека. PORT1, PORT2 - это порты, через которые сервер взаимодействует с читателями.

Читатель - отдельная программа на С с названием `reader`, которую следует запускать с теми же аргументами командной строки, а также с дополнительным аргументом IP, который представляет собой IP адрес сервера, с которым следует взаимодействовать читателям:

`./reader N PORT1 PORT2 IP`

**В случае, если проверяется программа на 6-7, 8, 9 баллов, аргументы будут другими**, что будет представлено в отчете ниже по тексту.

###Библиотекарь
является отдельным процессом-сервером с двумя потоками. Первый поток предназначен для изменения содержимого библиотеки, а также ответов читателям в зависимости от их запроса и соответствующийх значений в массиве-библиотеке. То есть, первый поток отвечает за выдачу/возврат книг читателями через первый порт PORT1. Второй поток предназначен для передачи информации читателям о значении той или иной ячейки массива-библиотеки через порт PORT2, то есть второй поток отвечает за уведомления читателей о наличии/неналичии той или иной книги в библиотеке.

Примерный псевдокод библиотекаря представлен ниже:

Поток для выдачи/возврата книг

1. Создает сокет для прослушивания порта PORT1
2. Получает от пользователя число
3. Если число больше 0, то:
4. ____Уменьшает его на 1. Получаем индекс книги в `library`
5. ____Блокируем доступ к `library`
6. ____Если ячейка `library` по полученному индексу равна 0, то пишем в результат 0
7. ____Иначе пишем в результат 1
8. ____Разблокировать `library`
9. ____Отправляем результат клиенту
10. Иначе:
11. ____Получаем индекс книги для возврата
12. ____Блокируем `library`, увеличиваем на 1 ячейку и разблокировываем `library`
13. Вернуться к шагу 2

Поток для уведомления читателей

1. Создает сокет для прослушивания порта PORT2
2. Получаем от клиента число
3. Возвращаем значение `library` с индексом число-1, блокируя и разблокировывая доступ к `library`
4. Отправить результат, вернуться к шагу 2

Блокировка и разблокировка массива `library` на чтение/запись производится с помощью мьютекса pthread

###Читатель
является отдельным процессом-клиентом, работающим в одном потоке. Он действует следуя нижеприведенным шагам:

1. Случайным образом выбирает число от 1 до N - это книга, которую хочет прочитать читатель.
2. Процесс создает сокет, соединяется с сервером по первому порту и отправляет сгенерированное на первом шаге число, ожидая получить целое число в ответ. Если в ответ читатель получает число 1, то это означает, что библиотекарь отдал ему книгу. Если получил число 0, то библиотекарь книгу не отдал по причине ее отсутствия.
3. В случае, если читатель получил книгу, он её читает случайное (от 1 до 3 дней, где день - это 5 секунд) число дней. Если читатель не получил книгу, он циклически повторяет второй шаг, но на втором порту, пока не получит в ответ единицу и переходит к шагу 2, выполняя его на первом порту.
4. Прочитав книгу, читатель подключается к серверу по первому порту, передавая ему номер книги с отрицательным знаком. Это означает, что библиотекарь будет должен вернуть эту книгу в библиотеку. В этом случае библиотекарь не формирует и не отправляет клиенту ответ. Так как номера идут от 1 до N, то из-за неналичия нуля в запросе читателя данное поведение будет корректным.
5. Читатель повторяет цикл с шага 1.

Программа сервера и программы читатели завершаются, если на сервер подать сигнал SIGINT. Программа читатель завершается также и в том случае, если только процесс читателя получает сигнал SIGINT, но в таком случае он проверяет, не читает ли в данный момент книгу, и если читает, то возвращает её библиотекарю по первому порту, и далее завершается, если не читает, то просто завершается. Сервер работает даже при отсутствии читателей, читатель при отсутствии сервера завершает работу сразу после неудачной попытки соединиться с ним.

В дополнение ко всему вышеперечисленному, для более корректной работы с протоколом UDP, на программу-клиента возложена дополнительная обязанность по проверке получения сервером отправленных пакетов. Для этого в функции `sendAndGet` в цикле происходит попытка отправлять пакеты до тех пор, пока не будет получен положительный результат или пока не истратятся все попытки (задается константой `MAXTRIES` в файле `utils.h`). Это позволяет завершать любую из программ корректно и без ошибок: при получении сигнала SIGINT программа-клиент "вернет книгу" перед завершением (если таковая имеется, иначе просто завершится), а программа-сервер, получив SIGINT, просто завершится, при этом программы-клиенты не будут ожидать ответа от сервера вечно, а, истратя попытки на соединение, также завершатся.
