#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlTableModel>
#include <QSortFilterProxyModel>
#include <QSqlRelationalTableModel>
#include <QString>
#include <QDebug>
#include <QStringList>
#include <QtCharts>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void connectDb(const QString &f);
    void addEntry();
    void deleteEntry();
    double calculateBalance() const;
    void showChart();

private:
    Ui::MainWindow *ui;

    QString dbFileName;
    QSqlDatabase db;
    QSqlTableModel *dbModel;
    QSortFilterProxyModel *dbProxy;
};
#endif // MAINWINDOW_H
