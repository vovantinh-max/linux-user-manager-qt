#pragma once
// Palette matched to TCP_Socket ServerGUI.java and client/ui/UIUtils.java.
static const char *applicationTheme = R"QSS(
QWidget { font-family: 'DejaVu Sans'; font-size: 12px; color: #dae2ec; }
QMainWindow, QWidget#central, QWidget#content { background: #171d27; }
QFrame#sidebar { background: #1b222d; border: none; }
QFrame#sidebar QLabel { background: transparent; color: #a3b4c6; }
QLabel#brand { background: #345b56; color: #dae2ec; border-radius: 13px; font-size: 21px; font-weight: 700; }
QLabel#brandName { color: #dae2ec; font-size: 17px; font-weight: 700; padding-top: 7px; }
QLabel#navCaption { color: #91a0b5; font-size: 9px; padding-bottom: 6px; }
QLabel#serverInfo { color: #91a0b5; font-size: 10px; line-height: 1.5; padding-top: 14px; }
QPushButton#navButton, QPushButton#navActive { text-align: left; padding: 12px 9px; border: none; border-radius: 6px; color: #a3b4c6; background: transparent; }
QPushButton#navButton:hover { background: #26303f; color: #dae2ec; }
QPushButton#navActive { background: #2f3b4c; color: #dae2ec; font-weight: 600; }
QLabel#eyebrow { color: #91a0b5; font-size: 9px; font-weight: 600; }
QLabel#title { font-size: 22px; font-weight: 700; color: #dae2ec; }
QLabel#muted { color: #a3b4c6; font-size: 11px; }
QFrame#metricCard, QFrame#tableCard, QFrame#auditCard { background: #1f2733; border: 1px solid #374354; border-radius: 9px; }
QLabel#metricLabel { color: #a3b4c6; font-size: 10px; font-weight: 600; }
QLabel#metricHint { color: #91a0b5; font-size: 9px; }
QLabel#metricBlue, QLabel#metricPurple, QLabel#metricAmber { font-size: 27px; font-weight: 700; }
QLabel#metricBlue { color: #7797c2; }
QLabel#metricPurple { color: #a097c2; }
QLabel#metricAmber { color: #c4aa79; }
QLabel#sectionTitle { font-size: 14px; font-weight: 700; color: #dae2ec; }
QLabel#selectionLabel { color: #a3b4c6; font-size: 10px; }
QLabel#footerStatus { color: #91a0b5; font-size: 10px; }
QPushButton { background: #1f2733; border: 1px solid #374354; border-radius: 5px; padding: 9px 11px; color: #dae2ec; }
QPushButton:hover { background: #2f3b4c; border-color: #465468; }
QPushButton:pressed { background: #34445a; }
QPushButton:focus { border: 1px solid #77a49d; }
QPushButton:disabled { color: #91a0b5; background: #202835; border-color: #303c4d; }
QPushButton#primary { background: #7797c2; color: #171d27; border-color: #7797c2; font-weight: 600; }
QPushButton#primary:hover { background: #8aa8ce; }
QPushButton#danger { background: #382a34; border-color: #63434e; color: #cf8d93; }
QPushButton#danger:disabled { background: #2b2731; color: #a68c98; border-color: #463541; }
QLineEdit { background: #1b222d; border: 1px solid #374354; border-radius: 6px; padding: 9px; selection-background-color: #345b56; }
QLineEdit:focus { border-color: #77a49d; background: #1f2733; }
QTableWidget { background: #1f2733; border: none; selection-background-color: #2f3b4c; selection-color: #dae2ec; outline: none; }
QTableWidget::item { border-bottom: 1px solid #303c4d; padding: 9px; }
QTableWidget::item:focus { outline: none; }
QHeaderView::section { background: #26303f; color: #a3b4c6; padding: 11px 9px; border: none; font-size: 10px; font-weight: 600; }
QTextEdit { background: #1f2733; border: 1px solid #374354; border-radius: 5px; padding: 7px; }
QTextEdit#auditLog { border: none; background: #1b222d; font-family: 'DejaVu Sans Mono'; font-size: 10px; color: #acbacc; }
QScrollBar:vertical { width: 8px; background: transparent; margin: 0; }
QScrollBar::handle:vertical { background: #465468; border-radius: 4px; min-height: 24px; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QToolTip { background: #2f3b4c; color: #dae2ec; border: none; padding: 6px; }

QDialog, QMessageBox, QInputDialog { background: #1f2733; }
QDialog QLabel, QMessageBox QLabel { background: transparent; color: #dae2ec; }
QComboBox { background: #1b222d; color: #dae2ec; border: 1px solid #374354; border-radius: 5px; padding: 8px; }
QComboBox QAbstractItemView { background: #26303f; color: #dae2ec; selection-background-color: #345b56; }
QTableWidget QTableCornerButton::section { background: #26303f; border: none; }
QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: #1b222d; }
QScrollBar:horizontal { height: 8px; background: #1b222d; }
QScrollBar::handle:horizontal { background: #465468; border-radius: 4px; min-width: 24px; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
)QSS";
