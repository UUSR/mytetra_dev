#include <QMessageBox>
#include <QCommonStyle>

#include "EditorToolbarSettingsScreen.h"
#include "../../controllers/editorToolbarSettings/EditorToolbarAvailableCommandsController.h"
#include "../../controllers/editorToolbarSettings/EditorToolbarUsedCommandsController.h"
#include "main.h"
#include "views/mainWindow/MainWindow.h"
#include "libraries/wyedit/EditorConfig.h"

EditorToolbarSettingsScreen::EditorToolbarSettingsScreen(QWidget *parent) : QDialog(parent), needRestart(false), changedCommands(false)
{
    this->setWindowTitle(tr("Toolbars settings"));

    setupUI();
    setupSignals();
    assembly();

    // Выбор Toolbar 1
    selectToolbarsListWidget->setCurrentRow(0);

    // Изначальные, в момент заргузки диалога списки команд
    // Используются для проверки на наличие изменений в расположении команд на понелях и нужде в перезагрузке
    loadedAvailableCommandsList = availableCommandsToolbarController->getModel()->getCommandsList();
    loadedToolBar1CommandsList = usedCommandsToolbar1Controller->getModel()->getCommandsList();
    loadedToolBar2CommandsList = usedCommandsToolbar2Controller->getModel()->getCommandsList();

    // Задание размеров диалога
    this->setMinimumHeight(
        int( 0.5 * static_cast<double>(find_object<MainWindow>("mainwindow")->height()) )
    );
    this->resize(this->size());
}


EditorToolbarSettingsScreen::~EditorToolbarSettingsScreen()
{

}

// Возвращает признак необходимости перезагрузки MyTetra, в зависимости от уровеня сложности вносимых изменений
bool EditorToolbarSettingsScreen::isNeedRestart() const
{
    // false - изменения можно делать на лету, перезагрузка MyTetra не нужна
    // true - для принятия изменений нужна перезагрузка MyTetra
    return needRestart;
}

void EditorToolbarSettingsScreen::setupUI()
{
    // Доступные команды панелей инструментов
    availableToolbarsCommandsLabel = new QLabel(tr("Available Toolbars Commands"), this);

    // Виджет выбора панели инструментов
    selectToolbarsListWidget = new QListWidget(this);
    selectToolbarsListWidget->setAutoFillBackground(true);
    selectToolbarsListWidget->addItem(tr("Toolbar 1"));
    selectToolbarsListWidget->addItem(tr("Toolbar 2"));
    QFontMetrics metr(selectToolbarsListWidget->font());
    selectToolbarsListWidget->setFixedHeight(metr.height()*3);

    // Невидимая надпись над кнопками перемещения, нужна для вертикального выравнивания
    exchangeButtonsLabel=new QLabel(" ");

    // Кнопки для перемещения выбранных кнопок панелей
    toUsedCommandsPushButton = new QPushButton(QIcon(":/resource/pic/move_right.svg"), "", this);
    toUsedCommandsPushButton->setShortcut(QKeySequence("Alt+Right"));

    toAvailableCommandsPushButton = new QPushButton(QIcon(":/resource/pic/move_left.svg"), "", this);
    toAvailableCommandsPushButton->setShortcut(QKeySequence("Alt+Left"));

    usedCommandUpPushButton = new QPushButton(QIcon(":/resource/pic/move_up.svg"), "", this);
    usedCommandUpPushButton->setShortcut(QKeySequence("Alt+Up"));

    usedCommandDownPushButton = new QPushButton(QIcon(":/resource/pic/move_dn.svg"), "", this);
    usedCommandDownPushButton->setShortcut(QKeySequence("Alt+Down"));
    // Текущие комманды на панели инструментов
    usedCommandsToolbarLabel = new QLabel(tr("Current Toolbar Commands"), this);

    // Контроллер списка всех доступных кнопок Редактора Текста
    availableCommandsToolbarController = new EditorToolbarAvailableCommandsController(this);
    availableCommandsToolbarController->setObjectName("editorToolbarAvailableCommandsController");
    availableCommandsToolbarController->init();

    // Контроллер списка кнопок Панели 1 Редактора Текста
    usedCommandsToolbar1Controller = new EditorToolbarUsedCommandsController(GlobalParameters::EditorToolbar::First, this);
    usedCommandsToolbar1Controller->setObjectName("editorToolbarUsedCommands1Controller");
    usedCommandsToolbar1Controller->init();

    // Контроллер списка кнопок Панели 2 Редактора Текста
    usedCommandsToolbar2Controller = new EditorToolbarUsedCommandsController(GlobalParameters::EditorToolbar::Second, this);
    usedCommandsToolbar2Controller->setObjectName("editorToolbarUsedCommands2Controller");
    usedCommandsToolbar2Controller->init();

    // Создание информационной кнопки
    QCommonStyle styleHelp;
    infoPushButton = new QPushButton(styleHelp.standardIcon(QStyle::SP_MessageBoxQuestion), "", this);

    // Создание набора диалоговых кнопок
    dialogButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
}


void EditorToolbarSettingsScreen::setupSignals()
{
    // Обработка переключения обрабатываемых панелей
    connect(selectToolbarsListWidget, &QListWidget::currentItemChanged,
            this,                     &EditorToolbarSettingsScreen::onCheckViewToolbarWidget); // Переключение видимости виджетов со списком кнопок

    // Обработка кнопок перемещения команд
    connect(toUsedCommandsPushButton, &QPushButton::clicked, // Перемещение выбранной команды из списка всех достукпных команд в список команд выбранной панели
            this,            &EditorToolbarSettingsScreen::onMoveAvailableCommandToUsedCommands);

    connect(toAvailableCommandsPushButton, &QPushButton::clicked, // Перемещение выбранной команды из списка команд выбранной панели в список всех достукпных команд
            this,           &EditorToolbarSettingsScreen::onMoveUsedCommandToAvailableCommands);

    connect(usedCommandUpPushButton, &QPushButton::clicked, // Перемещение выбранной команды вверх в списке команд выбранной панели
            this,         &EditorToolbarSettingsScreen::onMoveSelectedCommandUp);

    connect(usedCommandDownPushButton, &QPushButton::clicked, // Перемещение выбранной команды вниз в списке команд выбранной панели
            this,           &EditorToolbarSettingsScreen::onMoveSelectedCommandDown);

    // Вызов формы с информацией о формате ввода горячих клавиш
    connect(infoPushButton, &QPushButton::clicked,
            this,           &EditorToolbarSettingsScreen::onInfoClick);

    // Обработка кнопки OK
    connect(dialogButtonBox, &QDialogButtonBox::accepted,
            this,            &EditorToolbarSettingsScreen::applyChanges); // Применение изменений

    // Обработка кнопки Cancel
    connect(dialogButtonBox, &QDialogButtonBox::rejected, this, &EditorToolbarSettingsScreen::close);
}


void EditorToolbarSettingsScreen::assembly()
{
    // Слой, объединяющий кнопки для перемещения выбранных кнопок панелей
    buttonsToMoveLayout = new QVBoxLayout();
    buttonsToMoveLayout->addWidget(toUsedCommandsPushButton);
    buttonsToMoveLayout->addWidget(toAvailableCommandsPushButton);
    buttonsToMoveLayout->addWidget(usedCommandUpPushButton);
    buttonsToMoveLayout->addWidget(usedCommandDownPushButton);
    buttonsToMoveLayout->addStretch(1);
    buttonsToMoveLayout->addWidget(infoPushButton);

    // Стэк виджетов для видов 2-х панелей инструментов
    usedCommandsToolbarStackedWidget = new QStackedWidget();
    usedCommandsToolbarStackedWidget->addWidget(usedCommandsToolbar1Controller->getView());
    usedCommandsToolbarStackedWidget->addWidget(usedCommandsToolbar2Controller->getView());

    availableLayout=new QVBoxLayout();
    availableLayout->addWidget(availableToolbarsCommandsLabel);
    availableLayout->addWidget(availableCommandsToolbarController->getView());

    exchangeButtonsLayout=new QVBoxLayout();
    exchangeButtonsLayout->addWidget(exchangeButtonsLabel);
    exchangeButtonsLayout->addLayout(buttonsToMoveLayout);

    usedLayout=new QVBoxLayout();
    usedLayout->addWidget(usedCommandsToolbarLabel);
    usedLayout->addWidget(usedCommandsToolbarStackedWidget);

    exchangeLayout=new QHBoxLayout();
    exchangeLayout->addLayout(availableLayout);
    exchangeLayout->addLayout(exchangeButtonsLayout);
    exchangeLayout->addLayout(usedLayout);

    screenLayout=new QVBoxLayout();
    screenLayout->addWidget(selectToolbarsListWidget);
    screenLayout->addLayout(exchangeLayout);
    screenLayout->addWidget(dialogButtonBox);

    setLayout(screenLayout);
}

// Переключение виджетов с кнопками для конкретной панели инструментов редактора текста
void EditorToolbarSettingsScreen::onCheckViewToolbarWidget()
{
    usedCommandsToolbarStackedWidget->setCurrentIndex(selectToolbarsListWidget->currentRow());

    QString text = QString("%1 %2")
            .arg(tr("Commands for"))
            .arg(selectToolbarsListWidget->item(selectToolbarsListWidget->currentRow())->text());

    usedCommandsToolbarLabel->setText(text);
}

// Перемещение выбранной команды из модели всех доступных команд в модель команд рабочей панели
void EditorToolbarSettingsScreen::onMoveAvailableCommandToUsedCommands()
{
    // Получение индекса выбранной команды из списка всех доступных комманд
    QModelIndex selectedAvailableIndex = availableCommandsToolbarController->getSelectionModel()->currentIndex();

    // Код команды
    QString command = selectedAvailableIndex.row() == 0
            ? "separator" // Для <SEPARATOR> меняем название
            : selectedAvailableIndex.data(Qt::DisplayRole).toString();

    // Определение контроллера панели инструментов для работы с моделью
    EditorToolbarUsedCommandsController *controller = getCurrentEditorToolbarUsedCommandsController();

    // Индекс выбранного элемента списка команд панели инструментов
    QModelIndex toolbarSelectedIndex = controller->getSelectionModel()->currentIndex();
    int row = toolbarSelectedIndex.row();
    int newRow = 0; // Если элемент в списке панели инструментов не выбран, то вставляем в начало списка
    if (row > -1) {
        newRow = row;
    }

    // Проверка, есть ли добавляемый элемент в моделях всех панелей (separator не проверяем, их может быть любое число)
    if (command != "separator") {
        QModelIndex commandIndex1 = usedCommandsToolbar1Controller->getModel()->findCommand(command);
        if (commandIndex1!=QModelIndex()) {
            // Нашли такую же команду в ToolBar 1
            QMessageBox *msbox = new QMessageBox(
                        QMessageBox::Icon::Warning, "mytetra",
                        tr("<b>%1:</b> This command is already in <b>%2</b>.").arg(command).arg(tr("ToolBar 1")),
                        QMessageBox::Ok, this
            );
            msbox->exec();
            return;
        }

        QModelIndex commandIndex2 = usedCommandsToolbar2Controller->getModel()->findCommand(command);
        if (commandIndex2!=QModelIndex()) {
            // Нашли такую же команду в ToolBar 2
            QMessageBox msbox;
            msbox.setText(tr("<b>%1</b>: This command is already in <b>%2</b>.").arg(command).arg(tr("ToolBar 2")));
            msbox.setIcon(QMessageBox::Icon::Warning);
            msbox.setStandardButtons(QMessageBox::Ok);
            msbox.exec();
            return;
        }
    }

    // Создание нового item для добавления в список выбранной панели инструментов
    QStandardItem *newItem = new QStandardItem();
    newItem->setData(command, Qt::DisplayRole);

    // Добавление новой команды в модель
    controller->getModel()->insertRow(newRow, newItem);
    controller->getView()->scrollTo(newItem->index());

    // Выделение добавленной команды
    if (newRow == -1) {
        toolbarSelectedIndex = controller->getModel()->index(0, 0);
    }
    controller->getSelectionModel()->select(
                toolbarSelectedIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows
    );

    // Для добавленного в модель панели команды
    if (newItem->index().isValid()) {
        // Выделение добавленной команды
        controller->getSelectionModel()->select(
                    newItem->index(), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows
        );

        // Фокус на перемещенную команду
        controller->getView()->setCurrentIndex(newItem->index());
    }

    // Удаление команды из модели списка всех достукпных команд
    if (selectedAvailableIndex.row() != 0) {
        // Удаление всех элементов, кроме <SEPARATOR> из модели всех доступных команд
        availableCommandsToolbarController->getModel()->removeRow(selectedAvailableIndex.row());
    }
}

// Перемещение выбранной команды из модели команд рабочей панели в модель всех доступных команд
void EditorToolbarSettingsScreen::onMoveUsedCommandToAvailableCommands()
{
    // Определение контроллера для работы с моделью
    EditorToolbarUsedCommandsController *controller = getCurrentEditorToolbarUsedCommandsController();

    // Получение индекса выбранной команды из списка используемых комманд
    QModelIndex selectedIndex = controller->getSelectionModel()->currentIndex();

    // Код команды
    QString command(selectedIndex.data(Qt::DisplayRole).toString());

    // В модели должна остаться хотя бы одна команда
    if (controller->getModel()->rowCount() == 1) {
        return;
    }

    // <SEPARATOR> не добавляется в список всех доступных команд
    if (command == "separator") {
        // Удаление separator из списка команд панели инструментов
        controller->getModel()->removeRow(selectedIndex.row());

        // Выделение <SEPARATOR> в списке доступных команд
        QModelIndex separatorIndex = availableCommandsToolbarController->getModel()->index(0,0);
        availableCommandsToolbarController->getSelectionModel()->select(
            separatorIndex, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows
        );

        // Фокус на <SEPARATOR> в списке доступных команд
        availableCommandsToolbarController->getView()->setCurrentIndex(separatorIndex);
    } else {
        // Создание нового item для добавления в модель выбранной панели инструментов
        QStandardItem *newItem = new QStandardItem();
        newItem->setData(command, Qt::DisplayRole);

        // Добавление нового элемента в модель
        availableCommandsToolbarController->getModel()->insertRow(0, newItem);
        availableCommandsToolbarController->getModel()->sort(0);
        availableCommandsToolbarController->getView()->scrollTo(newItem->index());

        // Для добавленного в модель всех доступных команд
        if (newItem->index().isValid()) {
            // Выделение добавленной команды
            availableCommandsToolbarController->getSelectionModel()->select(
                newItem->index(), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows
            );

            // Фокус на перемещенную команду
            availableCommandsToolbarController->getView()->setCurrentIndex(newItem->index());
        }

        // Удаление выбранной команды из списка команд панели инструментов
        controller->getModel()->removeRow(selectedIndex.row());
    }
}

// Перемещение выбранной команды вверх в моделе команд рабочей панели инструментов
void EditorToolbarSettingsScreen::onMoveSelectedCommandUp()
{
    // Определение контроллера для работы с моделью
    EditorToolbarUsedCommandsController *controller = getCurrentEditorToolbarUsedCommandsController();

    // Перемещение выделенной команды вверх
    controller->moveCommandUpDown(EditorToolbarUsedCommandsController::CommandMove::Up);
}

// Перемещение выбранной команды вниз в моделе команд рабочей панели инструментов
void EditorToolbarSettingsScreen::onMoveSelectedCommandDown()
{
    // Определение контроллера для работы с моделью
    EditorToolbarUsedCommandsController *controller = getCurrentEditorToolbarUsedCommandsController();

    // Перемещение выделенной команды вниз
    controller->moveCommandUpDown(EditorToolbarUsedCommandsController::CommandMove::Down);
}

// Отображение информации-подсказки
void EditorToolbarSettingsScreen::onInfoClick()
{
    QString message = tr("<b>Information</b> for inserting the selected command (list of all available commands) in the list of working toolbars:");
    message += "<br/>";
    message += "<br/>";
    message += tr("The command is inserted <b>above the cursor</b> in the command list of the text editor toolbar.");
    message += "<br/>";
    message += "<br/>";
    message += "<b>Shortcuts:</b>";
    message += "<br/>";
    message += "- Move the selected command from the All Available Commands Model <b>to</b> the Commands Used Toolbar: <b>Alt+Right</b>.";
    message += "<br/>";
    message += "- Move the selected command from the Commands Used Toolbar <b>to</b> the All Available Commands Model: <b>Alt+Left</b>.";
    message += "<br/>";
    message += "- Commands used toolbar: Move <b>up</b> the selected command: <b>Alt+Up</b>.";
    message += "<br/>";
    message += "- Commands used toolbar: Move <b>down</b> the selected command: <b>Alt+Down</b>.";

    QMessageBox *msgBox = new QMessageBox(QMessageBox::Icon::Information, tr("Information"), message, QMessageBox::Ok, this);
    msgBox->exec();
}

// Сохранение результата распределения команд по панелям инструментов
void EditorToolbarSettingsScreen::applyChanges()
{
    // Измененные списки команд панелей инструментов
    QString availableCommandsList(availableCommandsToolbarController->getModel()->getCommandsList());
    QString toolBar1CommandsList(usedCommandsToolbar1Controller->getModel()->getCommandsList());
    QString toolBar2CommandsList(usedCommandsToolbar2Controller->getModel()->getCommandsList());

    // Проверка, были ли внесены изменения в расположение команд
    if (loadedAvailableCommandsList!=availableCommandsList ||
            loadedToolBar1CommandsList!=toolBar1CommandsList ||
            loadedToolBar2CommandsList!=toolBar2CommandsList ) {
        // Изменения были сделаны. Обработка изменений
        if (toolBar1CommandsList.indexOf("settings")==-1 && toolBar2CommandsList.indexOf("settings")==-1) {
            // Команду Настройки (settings) оставляем, иначе невозможно будет настраивать редактор
            QMessageBox msbox;
            msbox.setText(tr("The Settings command <b>%1</b> must be on the ToolBar 1 or ToolBar 2!").arg("settings"));
            msbox.setIcon(QMessageBox::Icon::Warning);
            msbox.setStandardButtons(QMessageBox::Ok);
            msbox.exec();

            needRestart = false; // изменения можно делать на лету, перезагрузка MyTetra не нужна
        } else {
            // Редактор конфигурации
            EditorConfig *editorConfig = getCurrentEditorToolbarUsedCommandsController()->getModel()->getEditorConfig();

            editorConfig->set_tools_line_1(toolBar1CommandsList);
            editorConfig->set_tools_line_2(toolBar2CommandsList);
            editorConfig->sync();

            needRestart = true; // Нужна перезагрузка MyTetra
            close();
        }
    } else {
        needRestart = false; // изменения можно делать на лету, перезагрузка MyTetra не нужна
        close();
    }
}

// Контроллер для работы с моделью панели используемых команд, в зависимости от выбранной панели
EditorToolbarUsedCommandsController *EditorToolbarSettingsScreen::getCurrentEditorToolbarUsedCommandsController()
{
    return usedCommandsToolbarStackedWidget->currentIndex() == 0
            ? usedCommandsToolbar1Controller : usedCommandsToolbar2Controller;
}
