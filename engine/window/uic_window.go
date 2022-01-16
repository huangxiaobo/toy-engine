package window

import (
	"github.com/therecipe/qt/core"
	"github.com/therecipe/qt/widgets"
)

type __mainwindow struct{}

func (*__mainwindow) init() {}

type MainWindow struct {
	*__mainwindow
	*widgets.QMainWindow
	Centralwidget    *widgets.QWidget
	GridLayout_2     *widgets.QGridLayout
	HorizontalLayout *widgets.QHBoxLayout
	TreeWidget       *widgets.QTreeWidget
	OpenGLWidget     *widgets.QOpenGLWidget
	Menubar          *widgets.QMenuBar
	Statusbar        *widgets.QStatusBar
}

func NewMainWindow(p widgets.QWidget_ITF) *MainWindow {
	var par *widgets.QWidget
	if p != nil {
		par = p.QWidget_PTR()
	}
	w := &MainWindow{QMainWindow: widgets.NewQMainWindow(par, 0)}
	w.setupUI()
	w.init()
	return w
}
func (w *MainWindow) setupUI() {
	if w.ObjectName() == "" {
		w.SetObjectName("MainWindow")
	}
	w.Resize2(969, 683)
	sizePolicy := widgets.NewQSizePolicy2(widgets.QSizePolicy__Maximum, widgets.QSizePolicy__Maximum, 0)
	sizePolicy.SetHorizontalStretch(0)
	sizePolicy.SetVerticalStretch(0)
	sizePolicy.SetHeightForWidth(w.SizePolicy().HasHeightForWidth())
	w.SetSizePolicy(sizePolicy)
	w.SetMinimumSize(core.NewQSize2(600, 400))
	w.Centralwidget = widgets.NewQWidget(w, 0)
	w.Centralwidget.SetObjectName("centralwidget")
	w.Centralwidget.SetEnabled(true)
	sizePolicy1 := widgets.NewQSizePolicy2(widgets.QSizePolicy__Expanding, widgets.QSizePolicy__Expanding, 0)
	sizePolicy1.SetHorizontalStretch(0)
	sizePolicy1.SetVerticalStretch(0)
	sizePolicy1.SetHeightForWidth(w.Centralwidget.SizePolicy().HasHeightForWidth())
	w.Centralwidget.SetSizePolicy(sizePolicy1)
	w.GridLayout_2 = widgets.NewQGridLayout(w.Centralwidget)
	w.GridLayout_2.SetObjectName("gridLayout_2")
	w.HorizontalLayout = widgets.NewQHBoxLayout()
	w.HorizontalLayout.SetObjectName("horizontalLayout")
	w.HorizontalLayout.SetSizeConstraint(widgets.QLayout__SetDefaultConstraint)
	w.TreeWidget = widgets.NewQTreeWidget(w.Centralwidget)
	__qtreewidgetitem := widgets.NewQTreeWidgetItem(0)
	__qtreewidgetitem.SetText(0, "1")
	w.TreeWidget.SetHeaderItem(__qtreewidgetitem)
	w.TreeWidget.SetObjectName("treeWidget")
	w.TreeWidget.SetAutoExpandDelay(0)
	w.TreeWidget.SetIndentation(10)
	w.HorizontalLayout.QLayout.AddWidget(w.TreeWidget)
	w.OpenGLWidget = widgets.NewQOpenGLWidget(w.Centralwidget, 0)
	w.OpenGLWidget.SetObjectName("openGLWidget")
	sizePolicy2 := widgets.NewQSizePolicy2(widgets.QSizePolicy__Preferred, widgets.QSizePolicy__Expanding, 0)
	sizePolicy2.SetHorizontalStretch(0)
	sizePolicy2.SetVerticalStretch(0)
	sizePolicy2.SetHeightForWidth(w.OpenGLWidget.SizePolicy().HasHeightForWidth())
	w.OpenGLWidget.SetSizePolicy(sizePolicy2)
	w.HorizontalLayout.QLayout.AddWidget(w.OpenGLWidget)
	w.HorizontalLayout.SetStretch(0, 1)
	w.HorizontalLayout.SetStretch(1, 5)
	w.GridLayout_2.AddLayout2(w.HorizontalLayout, 0, 0, 1, 1, 0)
	w.SetCentralWidget(w.Centralwidget)
	w.Menubar = widgets.NewQMenuBar(w)
	w.Menubar.SetObjectName("menubar")
	w.Menubar.SetGeometry(core.NewQRect4(0, 0, 969, 23))
	w.SetMenuBar(w.Menubar)
	w.Statusbar = widgets.NewQStatusBar(w)
	w.Statusbar.SetObjectName("statusbar")
	w.SetStatusBar(w.Statusbar)
	w.retranslateUi()
	core.QMetaObject_ConnectSlotsByName(w)

}
func (w *MainWindow) retranslateUi() {
	w.SetWindowTitle(core.QCoreApplication_Translate("MainWindow", "MainWindow", "", 0))

}
