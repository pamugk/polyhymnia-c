# Polyhymnia

## О проекте

Разработка проекта идёт исключительно для того, чтобы, во-первых, посмотреть, как может вестись разработка музыкальных проигрывателей, с какими проблемами можно столкнуться; во-вторых, чтобы попрактиковаться в разработке приложений на основе GTK. Какого-то серьёзного развития проекта **пока** не планируется.

Приоритеты при разработке приложения:
1. Производительность и небольшое потребление ресурсов.
2. Удобный (хотя бы лично для разработчика) и современный (на момент разработки) интерфейс.
3. Охват основной функциональности, ожидаемое от музыкального проигрывателя.

Обзор интерфейса см. [тут](./UI.md).

## Реализованные возможности

На данный момент реализованы следующие возможности:
* запуск обновления базы песен,
* просмотр базы песен,
* управление воспроизведением песен,
* управление очередью воспроизведения,
* управление плейлистами,
* просмотр статистики с сервера MPD,
* просмотр подробных сведений о песне.

## Возможности для расширения

Что ещё может быть сделано:
* замена MPD на собственный бэкенд для работы с базой песен (на основе [GStreamer](https://gstreamer.freedesktop.org/), возможно?),
* интеграция со внешними источниками для получения дополнительных данных о песнях ([Musicbrainz](https://musicbrainz.org/), например?),
* интеграция со внешними источниками для получения дополнительных данных об исполнителях и альбомах,
* интеграция со внешними источниками для получения текстов песен (или хотя бы просмотр текстов песен из метаданных песен),
* расширение возможностей настройки приложения (авто-обновление базы песен при запуске, возрастные ограничения на основе данных из внешних источников, и т.д.),
* система предложения песен (и новых исполнителей?) на основе истории прослушивания.

## Использованные технологии и инструментальные средства

Проигрыватель разработан на языке программирования C.

При разработке использованы следующие программные библиотеки:
* [GTK](https://gitlab.gnome.org/GNOME/gtk/) - основа для UI.
* [LibAdwaita](https://gitlab.gnome.org/GNOME/libadwaita) - дополнительные готовые UI-компоненты и стили.
* [libmpdclient](https://github.com/MusicPlayerDaemon/libmpdclient) - взаимодейтвие с MPD.

Для функционирования проигрывателя также необходимы следующие компоненты:
* [MPD](https://www.musicpd.org/) - управление базой песен и их воспроизведение.

## Использованные вспомогательные материалы

При разработке приложения были использованы следующие дополнительные материалы:
* [GNOME HIG](https://developer.gnome.org/hig/index.html) - набор указаний к разработке UI,
* [GNOME UI icons](https://developer.gnome.org/hig/guidelines/ui-icons.html) - набор UI иконок (в частности, пригодилось приложение [Icon Library](https://flathub.org/apps/org.gnome.design.IconLibrary)).

## Лицензия
Использование, модификация и распространение определено приложенной [лицензией](./LICENSE).