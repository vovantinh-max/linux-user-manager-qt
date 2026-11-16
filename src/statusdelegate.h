#pragma once
#include <QApplication>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <algorithm>

// Paint directly in the cell: no QLabel, no stylesheet margins, no duplicate text.
class StatusDelegate : public QStyledItemDelegate {
public:
    explicit StatusDelegate(QObject *parent=nullptr) : QStyledItemDelegate(parent) {}
    void paint(QPainter *painter, const QStyleOptionViewItem &option,
               const QModelIndex &index) const override {
        QStyleOptionViewItem opt(option);
        initStyleOption(&opt,index);
        const QString text=opt.text;
        opt.text.clear();
        auto style=opt.widget?opt.widget->style():QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem,&opt,painter,opt.widget);
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        QFont font=option.font; font.setPointSize(9);painter->setFont(font);
        const QFontMetrics metrics(font);
        const bool blocked=text==QString::fromUtf8("Đang khóa") || text==QString::fromUtf8("Hết hạn");
        const bool empty=text==QString::fromUtf8("Chưa có mật khẩu");
        const QColor fg=blocked||empty?QColor("#c4aa79"):QColor("#7db197");
        const QColor bg=blocked||empty?QColor("#3a342b"):QColor("#263d35");
        const int available=std::max(0,option.rect.width()-16);
        const int width=std::min(available,metrics.horizontalAdvance(text)+34);
        const int height=std::min(28,std::max(0,option.rect.height()-8));
        const QRect badge(option.rect.left()+8,option.rect.top()+(option.rect.height()-height)/2,width,height);
        painter->setPen(Qt::NoPen);painter->setBrush(bg);painter->drawRoundedRect(badge,6,6);
        painter->setBrush(fg);painter->drawEllipse(QPointF(badge.left()+11,badge.center().y()),3,3);
        painter->setPen(fg);
        const QRect label=badge.adjusted(22,0,-8,0);
        painter->drawText(label,Qt::AlignLeft|Qt::AlignVCenter,metrics.elidedText(text,Qt::ElideRight,std::max(0,label.width())));
        painter->restore();
    }
    QSize sizeHint(const QStyleOptionViewItem &option,const QModelIndex &index) const override {
        auto size=QStyledItemDelegate::sizeHint(option,index);size.setHeight(std::max(44,size.height()));return size;
    }
};
