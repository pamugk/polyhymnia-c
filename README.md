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
* просмотр подробных сведений о песне,
* просмотр текста песни.

## Возможности для расширения

Что ещё может быть сделано:

* замена MPD на собственный бэкенд для работы с базой песен (на основе [GStreamer](https://gstreamer.freedesktop.org/), возможно?),
* интеграция со внешними источниками для получения дополнительных данных о песнях ([Musicbrainz](https://musicbrainz.org/), например?),
* интеграция со внешними источниками для получения дополнительных данных об исполнителях и альбомах,
* интеграция со внешними источниками для получения текстов песен (или хотя бы просмотр текстов песен из метаданных песен),
* расширение возможностей настройки приложения (авто-обновление базы песен при запуске, возрастные ограничения на основе данных из внешних источников, и т.д.),
* система предложения песен (и новых исполнителей?) на основе истории прослушивания,
* поддержка синхронизации отображаемого текста песни с воспроизведением.

## Использованные технологии и инструментальные средства

Проигрыватель разработан на языке программирования C.

При разработке использованы следующие программные библиотеки:

* [GTK](https://gitlab.gnome.org/GNOME/gtk/) - основа для UI,
* [LibAdwaita](https://gitlab.gnome.org/GNOME/libadwaita) - дополнительные готовые UI-компоненты и стили,
* [libmpdclient](https://github.com/MusicPlayerDaemon/libmpdclient) - взаимодейтвие с MPD,
* [libsoup](https://libsoup.org/libsoup-3.0/index.html) - взаимодействие со внешними источниками данных в Интернете,
* [JSON-GLib](https://gnome.pages.gitlab.gnome.org/json-glib/) - обработка данных в формате [JSON](https://www.json.org/json-ru.html),
* [WebKitGtk](https://webkitgtk.org/) - встраивание веб-содержимого.

Для функционирования проигрывателя также необходимы следующие компоненты:

* [MPD](https://www.musicpd.org/) - управление базой песен и их воспроизведение.

## Рекомендации по сборке

### Система сборки

В качестве системы сборки используется [Meson](https://mesonbuild.com/). Требуемая версия: не ниже 1.2.

### Зависимости сборки

Приложение напрямую зависит от следующих библиотек:

* `gtk4 >= 4.11.5` ⚠️,
* `json-glib-1.0 >= 1.2`,
* `libadwaita-1 >= 1.5` ⚠️,
* `libmpdclient >= 2.20` ⚠️,
* `libsoup-3.0`,
* `webkitgtk-6.0`,

Зависимости, отмеченные знаком "⚠️", обязательны для функционирования базовых возможностей приложения. Также стоит помнить и о транзитивных зависимостях, требуемых основными зависимостями.
Прочие зависимости могут быть задействованы для включения в процессе сборки дополнительных возможностей приложения.

Дополнительная возможность включается в процессе сборки тогда и только тогда, когда доступны все необходимые для её функционирования зависимости.
Подробнее о зависимостях доп. возможностей см. ниже, в разделе ["Дополнительные возможности приложения"](дополнительные-возможности-приложения).

### Дополнительные возможности приложения

По умолчанию для включения дополнительных возможностей при сборке необходимо и достаточно наличие нужных библиотек.
Также можно указать состояние возможности (включена / отключена / автоопределение) через [опции сборки Meson](https://mesonbuild.com/Build-options.html).

| Возможность  | Опция сборки | Требуемые возможности | Зависимости |
| ------------ | ------------ | --------------------- | ----------- |
| Получение данных из внешних источников в Интернете  | `external_data_feature` | - | `json-glib-1.0`, `libsoup-3.0` |
| Поиск и отображение текстов песен  | `lyrics_feature` | `external_data_feature` | `webkitgtk-6.0` |

### Прочие опции сборки

Помимо опций сборки, контролирующих состояние доп. возможностей, также есть следующие опции, влияющие на функционирование приложения:

* `genius_client_access_token` (токен для доступа к API Genius).

## Используемые сторонние источники данных
Для получения дополнительных данных реализовано использование следующих источников:

* [Genius](https://genius.com) - тексты песен.

## Использованные вспомогательные материалы

При разработке приложения были использованы следующие дополнительные материалы:

* [GNOME HIG](https://developer.gnome.org/hig/index.html) - набор указаний к разработке UI,
* [GNOME UI icons](https://developer.gnome.org/hig/guidelines/ui-icons.html) - набор UI иконок (в частности, пригодилось приложение [Icon Library](https://flathub.org/apps/org.gnome.design.IconLibrary)).

## Лицензия
Использование, модификация и распространение определено приложенной [лицензией](./LICENSE).
