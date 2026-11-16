// SPDX-License-Identifier: MIT
#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QTextEdit>
#include <QMessageBox>
#include <QInputDialog>
#include <QProcess>
#include <QFile>
#include <QDir>
#include <QDate>
#include <QDateTime>
#include <QTextStream>
#include <QMap>
#include <QSet>
#include <QTimer>
#include <QAbstractItemView>
#include <QTextCursor>
#include <QFrame>
#include <QGridLayout>
#include <QStyle>
#include <QColor>
#include <QBrush>
#include <QStackedWidget>
#include <QDialog>
#include <QDialogButtonBox>
#include <algorithm>
#include <stdexcept>
#ifdef Q_OS_LINUX
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif
#include "policy.h"
#include "theme.h"
#include "statusdelegate.h"

struct User { QString name, home, shell; unsigned uid = 0, gid = 0; };
struct Group { QString name; unsigned gid = 0; QStringList members; };
struct Shadow { QString password; qint64 expires = -1; };

static void fail(const QString &text) { throw std::runtime_error(text.toUtf8().constData()); }
static QStringList readLines(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) fail("Không đọc được " + path + ": " + file.errorString());
    return QString::fromUtf8(file.readAll()).split('\n', QString::SkipEmptyParts);
}

class Accounts {
public:
    const QString logDirectory = "/var/log/linux-user-manager-qt";
    QStringList events;
    QString operatorName = QString::fromLocal8Bit(qgetenv("SUDO_USER"));
    bool auditReady = false;

    QMap<QString, User> users() const {
        QMap<QString, User> result;
        for (const auto &line : readLines("/etc/passwd")) {
            const auto f = line.split(':');
            if (f.size() >= 7) result.insert(f[0], {f[0], f[5], f[6], f[2].toUInt(), f[3].toUInt()});
        }
        return result;
    }
    QMap<QString, Group> groups() const {
        QMap<QString, Group> result;
        for (const auto &line : readLines("/etc/group")) {
            const auto f = line.split(':');
            if (f.size() >= 4) result.insert(f[0], {f[0], f[2].toUInt(), f[3].split(',', QString::SkipEmptyParts)});
        }
        return result;
    }
    Shadow shadow(const QString &name) const {
        // Hash is used only in memory to identify lock state; never shown or logged.
        for (const auto &line : readLines("/etc/shadow")) {
            const auto f = line.split(':');
            if (f.size() >= 8 && f[0] == name) return {f[1], f[7].isEmpty() ? -1 : f[7].toLongLong()};
        }
        fail("Không tìm thấy trạng thái tài khoản cục bộ.");
        return {};
    }
    QString state(const Shadow &s) const {
        const auto today = QDate(1970, 1, 1).daysTo(QDate::currentDate());
        if (s.expires >= 0 && s.expires <= today) return "Hết hạn";
        if (s.password.startsWith('!') || s.password.startsWith('*')) return "Đang khóa";
        return s.password.isEmpty() ? "Chưa có mật khẩu" : "Có mật khẩu";
    }
    void validate(const QString &name) const {
        if (!policy::validName(name.toStdString())) fail("Tên phải bắt đầu ugm_, sau đó là chữ thường; chỉ dùng a–z, số, _. Tối đa 31 ký tự.");
    }
    User requireUser(const QString &name) const {
        validate(name);
        const auto all = users();
        if (!all.contains(name)) fail("Tài khoản không còn tồn tại. Hãy làm mới bảng.");
        const auto u = all.value(name);
        if (!policy::canManage(name.toStdString(), u.uid, operatorName.toStdString()))
            fail("Không được sửa tài khoản hệ thống hoặc tài khoản đang gọi sudo.");
        return u;
    }
    QString execute(const QString &tool, const QStringList &args) const {
        QString executable;
        for (const auto &dir : {"/usr/sbin/", "/usr/bin/", "/sbin/", "/bin/"}) {
            const QString candidate = QString(dir) + tool;
            if (QFile::exists(candidate)) { executable = candidate; break; }
        }
        if (executable.isEmpty()) fail("Thiếu lệnh hệ thống: " + tool);
        QProcess proc;
        proc.setProgram(executable);
        proc.setArguments(args); // No shell; each input is a separate argument.
        proc.start();
        if (!proc.waitForStarted(5000)) fail("Không chạy được " + tool + ": " + proc.errorString());
        if (!proc.waitForFinished(20000)) {
            proc.kill(); proc.waitForFinished(3000);
            fail("Lệnh quá thời gian: " + tool + ". Hãy làm mới để kiểm tra trạng thái thực tế.");
        }
        if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) {
            auto error = QString::fromLocal8Bit(proc.readAllStandardError()).trimmed();
            fail(tool + ": " + (error.isEmpty() ? "Thao tác thất bại." : error));
        }
        return QString::fromLocal8Bit(proc.readAllStandardOutput());
    }
    int openLog() const {
#ifdef Q_OS_LINUX
        const QByteArray path = QFile::encodeName(logDirectory + "/events.log");
        const int fd = ::open(path.constData(), O_RDWR | O_CREAT | O_APPEND | O_NOFOLLOW | O_CLOEXEC, 0600);
        if (fd < 0) fail("Không mở được nhật ký.");
        struct stat info {};
        if (fstat(fd, &info) || !S_ISREG(info.st_mode) || info.st_uid != 0 || info.st_nlink != 1 || (info.st_mode & 0077)) {
            ::close(fd); fail("Tệp nhật ký không có quyền an toàn.");
        }
        return fd;
#else
        fail("Ứng dụng chỉ hỗ trợ Linux."); return -1;
#endif
    }
    void prepareAudit() {
#ifdef Q_OS_LINUX
        if (!QDir().mkpath(logDirectory)) fail("Không tạo được thư mục nhật ký.");
        struct stat info {};
        const auto path = QFile::encodeName(logDirectory);
        if (lstat(path.constData(), &info) || !S_ISDIR(info.st_mode) || info.st_uid != 0 || (info.st_mode & 0022))
            fail("Thư mục nhật ký không an toàn.");
        const int fd = openLog();
        QFile log;
        if (!log.open(fd, QIODevice::ReadOnly, QFileDevice::AutoCloseHandle)) { ::close(fd); fail("Không đọc được nhật ký."); }
        QTextStream text(&log); text.setCodec("UTF-8");
        while (!text.atEnd()) { events.append(text.readLine()); if (events.size() > 100) events.removeFirst(); }
        auditReady = true;
#endif
    }
    void note(QString message) {
        message.replace('\n', ' '); message.replace('\r', ' ');
        const auto line = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") + " | " + message;
        events.append(line);
        try {
            const int fd = openLog();
            QFile file;
            if (!file.open(fd, QIODevice::WriteOnly | QIODevice::Append, QFileDevice::AutoCloseHandle)) {
#ifdef Q_OS_LINUX
                ::close(fd);
#endif
                fail("Không ghi được nhật ký.");
            }
            const auto bytes = (line + '\n').toUtf8();
            if (file.write(bytes) != bytes.size() || !file.flush()) fail("Không lưu được nhật ký.");
#ifdef Q_OS_LINUX
            if (fsync(file.handle()) != 0) fail("Không đồng bộ được nhật ký.");
#endif
        } catch (const std::exception &) { events.append("CẢNH BÁO: không lưu được nhật ký; hãy kiểm tra trạng thái tài khoản bằng Làm mới."); }
        while (events.size() > 300) events.removeFirst();
    }
    void createUser(const QString &name) {
        validate(name);
        if (users().contains(name) || groups().contains(name)) fail("Tên người dùng hoặc nhóm đã tồn tại.");
        execute("useradd", {"-m", "-U", "-s", "/bin/bash", name});
        note("THÀNH CÔNG tạo tài khoản: " + name + "; cần đặt mật khẩu.");
    }
    void createGroup(const QString &name) {
        validate(name);
        if (groups().contains(name)) fail("Nhóm đã tồn tại.");
        execute("groupadd", {name}); note("THÀNH CÔNG tạo nhóm: " + name);
    }
    void addGroup(const QString &name, const QString &group) {
        requireUser(name); validate(group);
        if (!groups().contains(group)) fail("Nhóm chưa tồn tại.");
        execute("usermod", {"-aG", group, name}); note("THÀNH CÔNG thêm " + name + " vào " + group);
    }
    void lock(const QString &name) {
        requireUser(name); execute("usermod", {"-L", "-e", "1", name}); note("THÀNH CÔNG khóa: " + name);
    }
    void unlock(const QString &name) {
        requireUser(name);
        if (!policy::hasPassword(shadow(name).password.toStdString())) fail("Cần đặt mật khẩu trước khi mở khóa.");
        execute("usermod", {"-U", "-e", "", name}); note("THÀNH CÔNG mở khóa, bỏ ngày hết hạn: " + name);
    }
    void removeUser(const QString &name, const QString &confirmation) {
        requireUser(name);
        if (!policy::confirmDelete(name.toStdString(), confirmation.toStdString())) fail("Xác nhận chưa đúng. Chưa xóa tài khoản.");
        execute("userdel", {name}); note("THÀNH CÔNG xóa tài khoản, giữ home: " + name);
    }
};

class Window : public QMainWindow {
    Accounts accounts;
    QTableWidget *table;
    QLineEdit *search;
    QLabel *summary, *status;
    QLabel *userCount, *groupCount, *lockedCount, *selectionLabel;
    QTextEdit *log;
    QList<QPushButton *> actionButtons;
    QStackedWidget *pages;
    QTableWidget *groupTable=nullptr, *historyTable=nullptr;
    QLineEdit *groupSearch=nullptr, *historySearch=nullptr;
    QLabel *groupSummary=nullptr, *historySummary=nullptr;
    bool passwordBusy = false;
public:
    Window() {
        accounts.prepareAudit();
        setWindowTitle("Ứng dụng quản lý người dùng và nhóm trên Linux"); resize(1180, 780); setMinimumSize(1000, 690);
        auto central = new QWidget; central->setObjectName("central"); setCentralWidget(central);
        auto root = new QHBoxLayout(central); root->setContentsMargins(0,0,0,0); root->setSpacing(0);
        auto sidebar = new QFrame; sidebar->setObjectName("sidebar"); sidebar->setFixedWidth(176);
        auto side = new QVBoxLayout(sidebar); side->setContentsMargins(16,26,16,20); side->setSpacing(9);
        auto brand = new QLabel("LU"); brand->setObjectName("brand"); brand->setFixedSize(46,46);brand->setAlignment(Qt::AlignCenter);side->addWidget(brand);
        auto brandName = new QLabel("Quản lý người dùng\nvà nhóm Linux");brandName->setObjectName("brandName");side->addWidget(brandName);
        auto sectionLabel = new QLabel("KHÔNG GIAN QUẢN TRỊ");sectionLabel->setObjectName("navCaption");side->addSpacing(20);side->addWidget(sectionLabel);
        auto navUsers = new QPushButton("  Người dùng");navUsers->setObjectName("navActive");navUsers->setIcon(style()->standardIcon(QStyle::SP_ComputerIcon));side->addWidget(navUsers);
        auto navGroups = new QPushButton("  Xem nhóm");navGroups->setObjectName("navButton");navGroups->setIcon(style()->standardIcon(QStyle::SP_DirIcon));side->addWidget(navGroups);
        auto navLogs = new QPushButton("  Nhật ký");navLogs->setObjectName("navButton");navLogs->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));side->addWidget(navLogs);
        side->addStretch();
        auto serverInfo = new QLabel("LINUX\nTài khoản cục bộ\nC++  /  Qt Widgets");serverInfo->setObjectName("serverInfo");side->addWidget(serverInfo);
        root->addWidget(sidebar);
        auto content = new QWidget;content->setObjectName("content");root->addWidget(content,1);
        auto layout = new QVBoxLayout(content); layout->setContentsMargins(20,14,20,12); layout->setSpacing(8);
        auto breadcrumb = new QLabel("KHÔNG GIAN QUẢN TRỊ  /  NGƯỜI DÙNG");breadcrumb->setObjectName("eyebrow");layout->addWidget(breadcrumb);
        auto heading = new QHBoxLayout;
        auto title = new QLabel("Ứng dụng quản lý người dùng và nhóm trên Linux"); title->setObjectName("title"); title->setWordWrap(true); heading->addWidget(title,1);
        layout->addLayout(heading);
        auto subtitle=new QLabel("Quản lý tài khoản, nhóm và quyền truy cập trên Linux.");subtitle->setObjectName("muted");layout->addWidget(subtitle);
        auto cards = new QHBoxLayout;cards->setSpacing(12);
        auto card = [this,cards](const QString &caption, const QString &hint, QLabel *&value, const QString &accent){
            auto frame=new QFrame;frame->setObjectName("metricCard");auto box=new QVBoxLayout(frame);box->setContentsMargins(14,9,14,9);box->setSpacing(3);
            auto label=new QLabel(caption);label->setObjectName("metricLabel");box->addWidget(label);
            value=new QLabel("—");value->setObjectName(accent);box->addWidget(value);
            auto helper=new QLabel(hint);helper->setObjectName("metricHint");box->addWidget(helper);cards->addWidget(frame,1);
        };
        card("NGƯỜI DÙNG","Tài khoản thuộc phạm vi ugm_",userCount,"metricBlue");
        card("NHÓM","Nhóm cục bộ có tiền tố ugm_",groupCount,"metricPurple");
        card("KHÓA / HẾT HẠN","Tài khoản cần được kiểm tra",lockedCount,"metricAmber");layout->addLayout(cards);
        pages=new QStackedWidget;layout->addWidget(pages,1);
        auto tableCard=new QFrame;tableCard->setObjectName("tableCard");auto panel=new QVBoxLayout(tableCard);panel->setContentsMargins(12,10,12,10);panel->setSpacing(7);pages->addWidget(tableCard);
        auto panelHeading=new QHBoxLayout;auto panelTitle=new QLabel("Danh sách tài khoản");panelTitle->setObjectName("sectionTitle");panelHeading->addWidget(panelTitle);panelHeading->addStretch();
        auto userButton = new QPushButton("Tạo người dùng");userButton->setObjectName("primary");userButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));panelHeading->addWidget(userButton);panel->addLayout(panelHeading);
        auto tools = new QHBoxLayout;
        auto groupButton = new QPushButton("Tạo nhóm");
        auto groupsButton = new QPushButton("Danh sách nhóm"); auto refreshButton = new QPushButton("Làm mới");
        search = new QLineEdit; search->setPlaceholderText("Tìm kiếm theo tên tài khoản..."); search->setClearButtonEnabled(true);
        search->addAction(style()->standardIcon(QStyle::SP_FileDialogContentsView),QLineEdit::LeadingPosition);
        tools->addWidget(search,1);tools->addWidget(groupButton);tools->addWidget(groupsButton);refreshButton->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));tools->addWidget(refreshButton);panel->addLayout(tools);
        summary = new QLabel;summary->setObjectName("muted");panel->addWidget(summary);
        table = new QTableWidget(0, 4); table->setHorizontalHeaderLabels({"TÀI KHOẢN", "UID", "NHÓM", "TRẠNG THÁI"});
        table->setItemDelegateForColumn(3,new StatusDelegate(table));
        table->setSelectionBehavior(QAbstractItemView::SelectRows); table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers); table->verticalHeader()->hide();
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        table->setAlternatingRowColors(false); table->setShowGrid(false); table->verticalHeader()->setDefaultSectionSize(48);table->setMinimumHeight(90);
        table->horizontalHeader()->setHighlightSections(false);table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft|Qt::AlignVCenter);
        panel->addWidget(table, 1);
        selectionLabel=new QLabel("Chưa chọn tài khoản");selectionLabel->setObjectName("selectionLabel");panel->addWidget(selectionLabel);
        auto actions = new QGridLayout;actions->setHorizontalSpacing(8);actions->setVerticalSpacing(8);
        const QStringList labels = {"Chi tiết", "Thêm vào nhóm", "Đặt mật khẩu", "Khóa", "Mở khóa", "Xóa tài khoản"};
        for (int i=0; i<labels.size(); ++i) {
            auto button = new QPushButton(labels[i]); actions->addWidget(button,0,i); actionButtons.append(button);
            if (i==5) button->setObjectName("danger");
            connect(button, &QPushButton::clicked, this, [this, i]{ run([this, i]{ act(i); }); });
        }
        panel->addLayout(actions);
        auto auditCard=new QFrame;auditCard->setObjectName("auditCard");auto auditLayout=new QVBoxLayout(auditCard);auditLayout->setContentsMargins(14,10,14,10);auditLayout->setSpacing(5);
        auto auditHeading=new QHBoxLayout;auto auditTitle=new QLabel("Nhật ký hoạt động");auditTitle->setObjectName("sectionTitle");auditHeading->addWidget(auditTitle);auditHeading->addStretch();auto auditHint=new QLabel("Lưu trên hệ thống");auditHint->setObjectName("muted");auditHeading->addWidget(auditHint);auditLayout->addLayout(auditHeading);
        log = new QTextEdit;log->setObjectName("auditLog");log->setReadOnly(true);log->setFixedHeight(54);auditLayout->addWidget(log);layout->addWidget(auditCard);
        status = new QLabel;status->setObjectName("footerStatus"); status->setWordWrap(true); layout->addWidget(status);
        auto groupPage=makeDataPage("Danh sách nhóm",{"TÊN NHÓM","GID","SỐ THÀNH VIÊN","NHÓM CHÍNH","NHÓM PHỤ"},groupTable,groupSearch,groupSummary);
        pages->addWidget(groupPage);
        auto historyPage=makeDataPage("Nhật ký hoạt động",{"THỜI GIAN","KẾT QUẢ","NỘI DUNG THAO TÁC"},historyTable,historySearch,historySummary);
        pages->addWidget(historyPage);
        historyTable->horizontalHeader()->setSectionResizeMode(0,QHeaderView::ResizeToContents);
        historyTable->horizontalHeader()->setSectionResizeMode(1,QHeaderView::ResizeToContents);
        auto showPage=[this,navUsers,navGroups,navLogs,breadcrumb,auditCard](int index){
            pages->setCurrentIndex(index);auditCard->setVisible(index==0);
            const QList<QPushButton*> buttons={navUsers,navGroups,navLogs};
            for(int i=0;i<buttons.size();++i){buttons[i]->setObjectName(i==index?"navActive":"navButton");buttons[i]->style()->unpolish(buttons[i]);buttons[i]->style()->polish(buttons[i]);}
            breadcrumb->setText(QString("KHÔNG GIAN QUẢN TRỊ  /  ")+QStringList{"NGƯỜI DÙNG","NHÓM","NHẬT KÝ"}[index]);
            refresh();
        };
        connect(navUsers,&QPushButton::clicked,this,[showPage]{showPage(0);});
        connect(navGroups,&QPushButton::clicked,this,[showPage]{showPage(1);});
        connect(navLogs,&QPushButton::clicked,this,[showPage]{showPage(2);});
        auto createOnGroupPage=new QPushButton("Tạo nhóm");createOnGroupPage->setObjectName("primary");
        static_cast<QVBoxLayout*>(groupPage->layout())->insertWidget(1,createOnGroupPage,0,Qt::AlignRight);
        connect(createOnGroupPage,&QPushButton::clicked,groupButton,&QPushButton::click);
        connect(groupTable,&QTableWidget::cellDoubleClicked,this,[this](int row,int){run([this,row]{showGroupMembers(groupTable->item(row,0)->text());});});
        connect(userButton, &QPushButton::clicked, this, [this]{ run([this]{
            bool ok; const auto name=QInputDialog::getText(this,"Tạo tài khoản","Tên tài khoản (ví dụ ugm_ngan):",QLineEdit::Normal,"ugm_",&ok).trimmed();
            if(ok) { accounts.createUser(name); QMessageBox::information(this,"Đã tạo tài khoản","Đã tạo tài khoản Linux. Hãy chọn tài khoản và đặt mật khẩu."); }
        }); });
        connect(groupButton, &QPushButton::clicked, this, [this]{ run([this]{
            bool ok; const auto name=QInputDialog::getText(this,"Tạo nhóm","Tên nhóm (ví dụ ugm_linux):",QLineEdit::Normal,"ugm_",&ok).trimmed();
            if(ok) accounts.createGroup(name);
        }); });
        connect(groupsButton,&QPushButton::clicked,this,[showPage]{showPage(1);});
        connect(refreshButton,&QPushButton::clicked,this,[this]{refresh();});
        connect(search,&QLineEdit::textChanged,this,[this]{filter();});
        connect(table,&QTableWidget::itemSelectionChanged,this,[this]{updateButtons();});
        connect(table,&QTableWidget::cellDoubleClicked,this,[this](int,int){run([this]{act(0);});});
        refresh();
    }
private:
    QFrame *makeDataPage(const QString &title,const QStringList &headers,QTableWidget *&target,QLineEdit *&query,QLabel *&info) {
        auto frame=new QFrame;frame->setObjectName("tableCard");auto box=new QVBoxLayout(frame);box->setContentsMargins(16,14,16,14);box->setSpacing(12);
        auto label=new QLabel(title);label->setObjectName("sectionTitle");box->addWidget(label);
        auto tools=new QHBoxLayout;query=new QLineEdit;query->setPlaceholderText("Tìm trong tất cả các cột...");query->setClearButtonEnabled(true);tools->addWidget(query,1);
        auto reload=new QPushButton("Làm mới");reload->setIcon(style()->standardIcon(QStyle::SP_BrowserReload));tools->addWidget(reload);box->addLayout(tools);
        info=new QLabel;info->setObjectName("muted");info->setWordWrap(true);box->addWidget(info);
        target=new QTableWidget(0,headers.size());target->setHorizontalHeaderLabels(headers);target->setEditTriggers(QAbstractItemView::NoEditTriggers);target->setSelectionBehavior(QAbstractItemView::SelectRows);target->setSelectionMode(QAbstractItemView::SingleSelection);target->verticalHeader()->hide();target->verticalHeader()->setDefaultSectionSize(44);target->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);target->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft|Qt::AlignVCenter);target->setShowGrid(false);box->addWidget(target,1);
        connect(query,&QLineEdit::textChanged,this,[target,query]{filterData(target,query->text());});
        connect(reload,&QPushButton::clicked,this,[this]{refresh();});
        return frame;
    }
    static void filterData(QTableWidget *target,const QString &query) {
        for(int r=0;r<target->rowCount();++r){bool match=false;for(int c=0;c<target->columnCount();++c)if(target->item(r,c) && target->item(r,c)->text().contains(query,Qt::CaseInsensitive))match=true;target->setRowHidden(r,!match);}
    }
    static void appendData(QTableWidget *target,const QStringList &values) {
        const int r=target->rowCount();target->insertRow(r);for(int c=0;c<values.size();++c){auto item=new QTableWidgetItem(values[c]);item->setToolTip(values[c]);target->setItem(r,c,item);}
    }
    void refreshGroups() {
        const auto users=accounts.users();const auto groups=accounts.groups();groupTable->setRowCount(0);
        for(auto g=groups.cbegin();g!=groups.cend();++g){if(!g.key().startsWith("ugm_"))continue;QStringList primary;QSet<QString> members;
            for(auto u=users.cbegin();u!=users.cend();++u)if(u.value().gid==g.value().gid){primary<<u.key();members.insert(u.key());}
            for(const auto &name:g.value().members)members.insert(name);
            appendData(groupTable,{g.key(),QString::number(g.value().gid),QString::number(members.size()),primary.isEmpty()?"—":primary.join(", "),g.value().members.isEmpty()?"—":g.value().members.join(", ")});
        }
        groupSummary->setText(QString("%1 nhóm cục bộ ugm_ • Nhấp đúp một nhóm để xem bảng thành viên.").arg(groupTable->rowCount()));filterData(groupTable,groupSearch->text());
    }
    void refreshHistory() {
        QStringList lines;QFile file;const int fd=accounts.openLog();
        if(!file.open(fd,QIODevice::ReadOnly,QFileDevice::AutoCloseHandle)) {
#ifdef Q_OS_LINUX
            ::close(fd);
#endif
            fail("Không đọc được nhật ký hệ thống.");
        }
        QTextStream text(&file);text.setCodec("UTF-8");while(!text.atEnd()){lines<<text.readLine();if(lines.size()>2000)lines.removeFirst();}
        historyTable->setRowCount(0);
        for(auto it=lines.crbegin();it!=lines.crend();++it){const int split=it->indexOf(" | ");const auto time=split>=0?it->left(split):QString("—");const auto message=split>=0?it->mid(split+3):*it;
            const auto result=message.startsWith("THÀNH CÔNG")?QString("Thành công"):message.startsWith("LỖI")?QString("Lỗi"):message.startsWith("CẢNH BÁO")?QString("Cảnh báo"):QString("Thông tin");
            appendData(historyTable,{time,result,message});historyTable->item(historyTable->rowCount()-1,1)->setForeground(QColor(result=="Thành công"?"#7db197":result=="Lỗi"?"#cf8d93":"#c4aa79"));
        }
        historySummary->setText(QString("%1 bản ghi gần nhất (tối đa 2.000), mới nhất ở trên • Đọc từ tệp nhật ký hệ thống.").arg(lines.size()));filterData(historyTable,historySearch->text());
    }
    void showGroupMembers(const QString &name) {
        const auto groups=accounts.groups();if(!groups.contains(name))fail("Nhóm không còn tồn tại. Hãy làm mới.");const auto group=groups.value(name);const auto users=accounts.users();
        QDialog dialog(this);dialog.setWindowTitle("Thành viên nhóm "+name);dialog.resize(700,440);auto layout=new QVBoxLayout(&dialog);auto heading=new QLabel(name+" • GID "+QString::number(group.gid));heading->setObjectName("sectionTitle");layout->addWidget(heading);
        auto members=new QTableWidget(0,3);members->setHorizontalHeaderLabels({"TÀI KHOẢN","UID","VAI TRÒ TRONG NHÓM"});members->setEditTriggers(QAbstractItemView::NoEditTriggers);members->setSelectionBehavior(QAbstractItemView::SelectRows);members->verticalHeader()->hide();members->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);members->verticalHeader()->setDefaultSectionSize(42);layout->addWidget(members);
        QSet<QString> listed;for(auto u=users.cbegin();u!=users.cend();++u){bool primary=u.value().gid==group.gid;bool extra=group.members.contains(u.key());if(primary||extra){listed.insert(u.key());appendData(members,{u.key(),QString::number(u.value().uid),primary&&extra?"Chính và phụ":primary?"Nhóm chính":"Nhóm phụ"});}}
        for(const auto &member:group.members)if(!listed.contains(member))appendData(members,{member,"—","Thành viên được ghi trong /etc/group"});
        layout->addWidget(new QLabel(QString("%1 thành viên").arg(members->rowCount())));auto close=new QDialogButtonBox(QDialogButtonBox::Close);connect(close,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);layout->addWidget(close);dialog.exec();
    }
    void showLog() { log->setPlainText(accounts.events.join('\n')); log->moveCursor(QTextCursor::End); }
    void filter() {
        for(int row=0;row<table->rowCount();++row) table->setRowHidden(row,!table->item(row,0)->text().contains(search->text(),Qt::CaseInsensitive));
        updateButtons();
    }
    void updateButtons() {
        const int row=table->currentRow(); const bool selected=row>=0 && !table->isRowHidden(row) && !passwordBusy;
        for(auto b:actionButtons) b->setEnabled(selected);
        selectionLabel->setText(selected?"ĐANG CHỌN  •  "+table->item(row,0)->text():passwordBusy?"Đang chờ hoàn tất Terminal đặt mật khẩu...":"Chọn một tài khoản để xem và quản lý");
    }
    template<class F> void run(F work) {
        try { work(); }
        catch(const std::exception &e) { const auto error=QString::fromUtf8(e.what()); accounts.note("LỖI: "+error); QMessageBox::warning(this,"Thao tác chưa hoàn tất",error); }
        refresh();
    }
    void refresh() {
        const int selectedRow=table->currentRow(); const auto selected=selectedRow>=0?table->item(selectedRow,0)->text():QString();
        try {
            const auto users=accounts.users(); const auto groups=accounts.groups();
            // Prepare the complete new snapshot before changing displayed rows.
            QList<QStringList> rows;
            for(auto it=users.cbegin();it!=users.cend();++it) {
                const auto &u=it.value(); if(!u.name.startsWith("ugm_")) continue;
                QStringList member; for(auto g=groups.cbegin();g!=groups.cend();++g) if(g.value().gid==u.gid || g.value().members.contains(u.name)) member << g.key();
                rows.append(QStringList{u.name,QString::number(u.uid),member.join(", "),accounts.state(accounts.shadow(u.name))});
            }
            table->setRowCount(0);
            int locked=0;
            for(const auto &values:rows) {
                int r=table->rowCount();table->insertRow(r);
                for(int c=0;c<4;++c) {auto item=new QTableWidgetItem(values[c]);item->setToolTip(values[c]);table->setItem(r,c,item);}
                auto font=table->item(r,0)->font();font.setBold(true);table->item(r,0)->setFont(font);table->item(r,0)->setForeground(QColor("#9db8dd"));
                table->item(r,1)->setForeground(QColor("#a3b4c6"));
                const bool blocked=values[3]=="Đang khóa" || values[3]=="Hết hạn";
                if(blocked) ++locked;
                if(values[0]==selected)table->selectRow(r);
            }
            int count=0; for(auto g=groups.cbegin();g!=groups.cend();++g) if(g.key().startsWith("ugm_")) ++count;
            userCount->setText(QString::number(rows.size()));groupCount->setText(QString::number(count));lockedCount->setText(QString::number(locked));
            summary->setText(QString("%1 tài khoản  •  Dữ liệu cục bộ từ Linux  •  Phạm vi ugm_").arg(rows.size()));
            status->setText(rows.isEmpty()?"Chưa có tài khoản ugm_. Bấm Tạo người dùng để tạo tài khoản.":"Dữ liệu đã cập nhật từ Linux.");
            filter();
        } catch(const std::exception &e) { status->setText("Không tải được dữ liệu: "+QString::fromUtf8(e.what())); }
        try {refreshGroups();refreshHistory();}catch(const std::exception &e){status->setText("Không tải được nhóm/nhật ký: "+QString::fromUtf8(e.what()));}
        showLog(); updateButtons();
    }
    void act(int action) {
        const int row=table->currentRow();
        if(row<0 || table->isRowHidden(row)) fail("Hãy chọn một tài khoản trong bảng.");
        const auto name=table->item(row,0)->text(); accounts.requireUser(name);
        bool ok=false;
        if(action==0) QMessageBox::information(this,"Chi tiết "+name,accounts.execute("id",{name})+"\n"+accounts.execute("chage",{"-l",name}));
        else if(action==1) {
            QStringList names; const auto groups=accounts.groups(); for(auto it=groups.cbegin();it!=groups.cend();++it) if(it.key().startsWith("ugm_"))names<<it.key();
            if(names.isEmpty())fail("Hãy tạo nhóm trước.");
            const auto group=QInputDialog::getItem(this,"Thêm vào nhóm",name+" → nhóm:",names,0,false,&ok);
            if(ok)accounts.addGroup(name,group);
        } else if(action==2) setPassword(name);
        else if(action==3) {
            if(QMessageBox::question(this,"Khóa "+name,"Khóa mật khẩu và đặt ngày hết hạn. Không ngắt phiên đang chạy. Tiếp tục?",QMessageBox::Yes|QMessageBox::No,QMessageBox::No)==QMessageBox::Yes)accounts.lock(name);
        } else if(action==4) {
            if(QMessageBox::question(this,"Mở khóa "+name,"Mở khóa mật khẩu và bỏ ngày hết hạn tài khoản?",QMessageBox::Yes|QMessageBox::No,QMessageBox::No)==QMessageBox::Yes)accounts.unlock(name);
        } else if(action==5) {
            const auto text=QInputDialog::getText(this,"Xóa tài khoản","Giữ thư mục home. Nhóm riêng có thể bị Linux xóa.\nNhập chính xác DELETE "+name+":",QLineEdit::Normal,"",&ok);
            if(ok)accounts.removeUser(name,text);
        }
    }
    void setPassword(const QString &name) {
        if(passwordBusy)fail("Đang có một phiên đặt mật khẩu. Hãy hoàn tất Terminal trước.");
        QString program; QStringList args;
        if(QFile::exists("/usr/bin/xterm")) {program="/usr/bin/xterm";args=QStringList{"-e","/usr/bin/passwd",name};}
        else if(QFile::exists("/usr/bin/gnome-terminal")) {program="/usr/bin/gnome-terminal";args=QStringList{"--wait","--","/usr/bin/passwd",name};}
        else fail("Thiếu Terminal. Cài xterm hoặc tự chạy: sudo passwd "+name);
        QMessageBox::information(this,"Đặt mật khẩu","Bạn tự nhập và xác nhận mật khẩu trong Terminal sắp mở.\nKhông nhập mật khẩu vào ứng dụng. Sau khi xong, bấm Làm mới.\nNếu tài khoản hết hạn, cần Mở khóa sau khi đặt mật khẩu.");
        auto proc=new QProcess(this); passwordBusy=true;updateButtons();
        connect(proc,&QProcess::errorOccurred,this,[this,proc](QProcess::ProcessError error){
            if(error==QProcess::FailedToStart){accounts.note("Không mở được Terminal đặt mật khẩu.");passwordBusy=false;proc->deleteLater();refresh();}
        });
        connect(proc,QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),this,[this,proc,name](int code,QProcess::ExitStatus exit){
            accounts.note(QString("Terminal đặt mật khẩu cho %1 đã kết thúc (mã %2, %3). Kiểm tra trạng thái bằng Làm mới.").arg(name).arg(code).arg(exit==QProcess::NormalExit?"bình thường":"lỗi"));
            passwordBusy=false;proc->deleteLater();refresh();
        });
        proc->start(program,args);
        accounts.note("Đang chờ người dùng tự đặt mật khẩu: "+name);showLog();
    }
};

int main(int argc,char *argv[]) {
    QApplication app(argc,argv);
    app.setApplicationName("Linux User Manager");
    app.setApplicationDisplayName(QString::fromUtf8("Ứng dụng quản lý người dùng và nhóm trên Linux"));
#ifndef Q_OS_LINUX
    QMessageBox::critical(nullptr,"Cần Linux","Ứng dụng chỉ quản lý tài khoản trên Linux.");return 1;
#else
    if(geteuid()!=0){QMessageBox::information(nullptr,"Cần quyền quản trị","Mở Terminal trong thư mục dự án và chạy:\nsudo -H ./build/linux-user-manager");return 1;}
#endif
    app.setStyle("Fusion");
    app.setStyleSheet(QString::fromUtf8(applicationTheme));
    try { Window window;window.show();return app.exec(); }
    catch(const std::exception &e){QMessageBox::critical(nullptr,"Không khởi động được",QString::fromUtf8(e.what()));return 1;}
}
