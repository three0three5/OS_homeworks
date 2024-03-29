# ИДЗ 3 ОС

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
2. Создает сокет для соединения с очередным пользователем
3. Получает от пользователя число
4. Если число больше 0, то:
5. ____Уменьшает его на 1. Получаем индекс книги в `library`
6. ____Блокируем доступ к `library`
7. ____Если ячейка `library` по полученному индексу равна 0, то пишем в результат 0
8. ____Иначе пишем в результат 1
9. ____Разблокировать `library`
10. ____Отправляем результат клиенту
11. Иначе:
12. ____Получаем индекс книги для возврата
13. ____Блокируем `library`, увеличиваем на 1 ячейку и разблокировываем `library`
14. Закрыть сокет, вернуться к шагу 2

Поток для уведомления читателей

1. Создает сокет для прослушивания порта PORT2
2. Создает сокет для соединения с очередным пользователем
3. Получаем от клиента число
4. Возвращаем значение `library` с индексом число-1, блокируя и разблокировывая доступ к `library`
5. Отправить результат, закрываем сокет, вернуться к шагу 2

Блокировка и разблокировка массива `library` на чтение/запись производится с помощью мьютекса pthread

###Читатель
является отдельным процессом-клиентом, работающим в одном потоке. Он действует следуя нижеприведенным шагам:

1. Случайным образом выбирает число от 1 до N - это книга, которую хочет прочитать читатель.
2. Процесс создает сокет, соединяется с сервером по первому порту и отправляет сгенерированное на первом шаге число, ожидая получить целое число в ответ. Если в ответ читатель получает число 1, то это означает, что библиотекарь отдал ему книгу. Если получил число 0, то библиотекарь книгу не отдал по причине ее отсутствия.
3. В случае, если читатель получил книгу, он её читает случайное (от 1 до 3 дней, где день - это 5 секунд) число дней. Если читатель не получил книгу, он циклически повторяет второй шаг, но на втором порту, пока не получит в ответ единицу и переходит к шагу 2, выполняя его на первом порту.
4. Прочитав книгу, читатель подключается к серверу по первому порту, передавая ему номер книги с отрицательным знаком. Это означает, что библиотекарь будет должен вернуть эту книгу в библиотеку. В этом случае библиотекарь не формирует и не отправляет клиенту ответ. Так как номера идут от 1 до N, то из-за неналичия нуля в запросе читателя данное поведение будет корректным.
5. Читатель повторяет цикл с шага 1.

Программа сервера и программы читатели завершаются, если на сервер подать сигнал SIGINT. Программа читатель завершается также и в том случае, если только процесс читателя получает сигнал SIGINT, но в таком случае он проверяет, не читает ли в данный момент книгу, и если читает, то возвращает её библиотекарю по первому порту, и далее завершается, если не читает, то просто завершается. Сервер работает даже при отсутствии читателей, читатель при отсутствии сервера завершает работу сразу после неудачной попытки соединиться с ним.

## 6-7, 8, 9

В дополнение к предыдущей программе добавляется программа `observer`, которая получает от библиотекаря и читателей лог их работы. При этом читатели теперь перед каждым своим сообщением имеют метку со случайным пятизначным числом для различия их в том числе и на локальной машине.

Программа запускается как

`./observer PORT3 IP`,

где PORT3 - третий вдобавок к предыдущим порт, через который будет взаимодействие с сервером, имеющим значение IP. При этом программа библиотекаря будет запускаться как

`./librarian N PORT1 PORT2 PORT3`,

соответственно.

Программы переработаны так, что теперь предыдущие функции библиотекаря получают не только число, но и идентификатор читателя от клиента. Получив информацию от читателя, сразу после этого библиотекарь вызывает функцию `sendAndForget()` со сформированной строкой для отправки постоянно подключенному к третьему порту observer'у, которая также вызывается после каждого вывода строки в консоль. Так как данная функция посылает по сокетам строки с высокой скоростью, для исключения некорректной передачи данных используется мьютекс `iolock`.

Реализация читателя изменилась так, что теперь он также передает серверу количество дней, которые читает книгу, для передачи этой информации observer'ам. Это потребовало не закрывать сокет сразу после отправки и получения информации в функции `getFromPort()`, а закрывать его после получения случайного числа дней и отправки такового серверу в main функции.

Случаи, когда сервер и клиент выполняют одинаковые действия (например, о передаче книги или ее отсутствии) не передаются в observer'ы для уменьшения количества текста, но также это упрощение не влияет на полноту получаемых данных о системе - по ним можно легко восстановить хронологию действий сервера и каждого читателя по отдельности. Таким образом, observer получает исчерпывающую информацию обо всём, что происходит между сервером и клиентами.

Программа может обрабатывать максимум 32 активных наблюдателя - дескрипторы сокетов каждого нового подключившегося наблюдателя заносятся в массив и удаляются из него в случае неудачной попытки вызова send() на сокет подключившегося наблюдателя. При этом выполняется требование на 8 баллов. Также программа позволяет корректно подключать и отключать читателей (после завершения читателя он возвращает книгу, если читал её, следовательно работоспособность других читателей и сервера позволит продолжать взаимодействие друг с другом), что одновременно выполняет требования на 9 баллов. Таким образом, программа в директории 6789 выполняет все необходимые требования для программы и на 6-7 баллов, и на 8 баллов, и на 9 баллов без потери требований на другие оценки, что позволяет использовать одну и ту же программу для каждой оценки (Формально, можно было бы скопировать программу 3 раза, и все требования были бы выполнены, однако это не осуществляется в целях экономии дискового пространства).

Демонстрация работы программы [Ссылка](https://disk.yandex.ru/d/-KuCm4PkUkmAMA)
