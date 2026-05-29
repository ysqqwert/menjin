#ifndef FORM_H
#define FORM_H

#include <QWidget>
#include <QImage>
namespace Ui {
class Form;
}

class Form : public QWidget
{
    Q_OBJECT

public:
    explicit Form(QWidget *parent = nullptr);
    ~Form();

    Ui::Form *ui;
};

#endif // FORM_H
