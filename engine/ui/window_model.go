package ui

import (
	"fmt"
	"github.com/go-gl/mathgl/mgl32"
	"github.com/inkyblackness/imgui-go/v4"
	"reflect"
)

type WindowMaterialItem struct {
	tag string
	key reflect.StructField
	val reflect.Value
}

var tabDetailHeader = []string{
	"Attribute", "Value",
}

var tabMaterialHeader = []string{
	"Attribute", "Value",
}

var tabGeometryHeader = []string{
	"Attribute", "Value",
}

type WindowModel struct {
	noClose bool
	visible bool
	flags   WindowFlags

	modelObj interface{}
	content  string

	showDemoWindow bool
}

func NewWindowModel() *WindowModel {
	w := &WindowModel{
		visible:  false,
		flags:    WindowFlags{noResize: true, noMenu: true, noTitlebar: true, noCollapse: true, noBackground: false},
		modelObj: nil,
	}

	return w
}

func (w *WindowModel) Reset() {
	w.modelObj = nil
}

//var colorUI = [3]float32{-1, -1, -1}
const (
	WindowModelWidth             = 360
	WindowModelTableColumnWidths = 100
	WindowModelTableColumn2Width = 200
	WindowModelItemWidth         = 200
)

func (w *WindowModel) Show(displaySize [2]float32) {
	if !w.visible || w.modelObj == nil {
		return
	}
	imgui.SetNextWindowPosV(imgui.Vec2{X: displaySize[0] - WindowModelWidth, Y: 0}, imgui.ConditionNone, imgui.Vec2{})
	imgui.SetNextWindowSizeV(imgui.Vec2{X: WindowModelWidth, Y: displaySize[1]}, imgui.ConditionNone)

	if !imgui.BeginV("ModelPanel", &w.visible, w.flags.combined()) {
		imgui.End()
		return
	}

	imgui.PushItemWidth(imgui.FontSize() * -12)

	rPtrType := reflect.TypeOf(w.modelObj)
	rPtrVal := reflect.ValueOf(w.modelObj)

	rMatType := rPtrType
	rMatVal := rPtrVal
	if rPtrType.Kind() == reflect.Ptr {
		rMatType = rPtrType.Elem()
		rMatVal = rPtrVal.Elem()
	}

	// set flags according to the options that have been selected
	flgs := imgui.TableFlagsNone
	flgs |= imgui.TableFlagsRowBg
	flgs |= imgui.TableFlagsBorders
	flgs |= imgui.TableFlagsNoBordersInBody
	flgs |= imgui.TableFlagsSizingStretchSame

	imgui.Spacing()
	imgui.Bullet()
	imgui.Text("Detail")
	imgui.Indent()

	if imgui.BeginTableV("tableMain", len(tabDetailHeader), flgs, imgui.Vec2{}, 0.0) {
		imgui.TableSetupColumnV("tableMain.Column1", imgui.TableColumnFlagsWidthFixed, WindowModelTableColumnWidths, 0)
		imgui.TableSetupColumnV("tableMain.Column2", imgui.TableColumnFlagsWidthFixed, WindowModelTableColumn2Width, 0)
		for _, fieldName := range []string{"Name", "Id"} {
			imgui.TableNextRow()
			imgui.TableSetColumnIndex(0)
			imgui.Text(fieldName)

			imgui.TableSetColumnIndex(1)
			imgui.SetNextItemWidth(WindowModelItemWidth)
			name := rMatVal.FieldByName(fieldName).String()
			imgui.Text(name)
		}
		imgui.EndTable()
	}

	imgui.Unindent()
	imgui.Spacing()
	imgui.Spacing()
	imgui.Bullet()
	imgui.Text("Geometry")
	imgui.Indent()

	if imgui.BeginTableV("tableGeometry", len(tabGeometryHeader), flgs, imgui.Vec2{}, 100.0) {
		imgui.TableSetupColumnV("tableGeometry.Column1", imgui.TableColumnFlagsWidthFixed, WindowModelTableColumnWidths, 0)
		imgui.TableSetupColumnV("tableGeometry.Column2", imgui.TableColumnFlagsWidthFixed, WindowModelTableColumn2Width, 0)

		for row, fieldName := range []string{"Position", "Scale", "Rotate"} {

			imgui.TableNextRow()
			imgui.TableSetColumnIndex(0)

			imgui.Text(fieldName)

			imgui.TableSetColumnIndex(1)
			imgui.SetNextItemWidth(WindowModelItemWidth)
			if row == 2 {
				w.ShowFloat(rPtrType, rPtrVal, fieldName)
			} else {
				w.ShowFloat3(rPtrType, rPtrVal, fieldName)
			}

		}
		imgui.EndTable()
	}

	imgui.Unindent()
	imgui.Spacing()
	imgui.Spacing()
	imgui.Bullet()
	imgui.Text("Material")
	imgui.Indent()

	material := rMatVal.FieldByName("Material").Interface()
	rMatType = reflect.TypeOf(material)
	rMatVal = reflect.ValueOf(material)
	//if rMatType.Kind() == reflect.Ptr {
	//	rMatType = rMatType.Elem()
	//	rMatVal = rMatVal.Elem()
	//}

	if imgui.BeginTableV("tableMaterial", len(tabMaterialHeader), flgs, imgui.Vec2{}, 0.0) {
		imgui.TableSetupColumnV("tableMaterial.Column1", imgui.TableColumnFlagsWidthFixed, WindowModelTableColumnWidths, 0)
		imgui.TableSetupColumnV("tableMaterial.Column2", imgui.TableColumnFlagsWidthStretch, WindowModelTableColumn2Width, 0)
		for row, fieldName := range []string{"AmbientColor", "DiffuseColor", "SpecularColor", "Shininess"} {

			imgui.TableNextRow()
			imgui.TableSetColumnIndex(0)

			imgui.Text(fieldName)

			imgui.TableSetColumnIndex(1)
			imgui.SetNextItemWidth(WindowModelItemWidth)
			if row == 3 {
				w.ShowFloat(rMatType, rMatVal, fieldName)
			} else {
				w.ShowColor3(rMatType, rMatVal, fieldName)
			}
		}

		imgui.EndTable()
	}

	imgui.Unindent()

	// End of ShowDemoWindow()
	imgui.End()

	if w.showDemoWindow {
		const demoX = 650
		const demoY = 20
		imgui.SetNextWindowPosV(imgui.Vec2{X: demoX, Y: demoY}, imgui.ConditionFirstUseEver, imgui.Vec2{})

		imgui.ShowDemoWindow(&w.showDemoWindow)
	}
}

func (w *WindowModel) ShowFloat3(rPtrType reflect.Type, rPtrVal reflect.Value, fieldName string) {
	rVal := rPtrVal.Elem().FieldByName(fieldName)
	if !rVal.IsValid() {
		return
	}
	f3 := rVal.Interface().(mgl32.Vec3)

	imgui.DragFloat3(fmt.Sprintf("##%s", fieldName), (*[3]float32)(&f3))

	methodName := fmt.Sprintf("Set%s", fieldName)
	method, err := rPtrType.MethodByName(methodName)
	if err != true {
		fmt.Printf("%v %v: %v\n", rPtrType, methodName, err)
		return
	}
	method.Func.Call([]reflect.Value{rPtrVal, reflect.ValueOf(f3)})
}

func (w *WindowModel) ShowFloat(rPtrType reflect.Type, rPtrVal reflect.Value, fieldName string) {
	rVal := rPtrVal.Elem().FieldByName(fieldName)
	if !rVal.IsValid() {
		return
	}

	f1 := rVal.Interface().(float32)

	imgui.DragFloat(fmt.Sprintf("##%s", fieldName), &f1)

	methodName := fmt.Sprintf("Set%s", fieldName)
	method, err := rPtrType.MethodByName(methodName)
	if err == true {
		method.Func.Call([]reflect.Value{rPtrVal, reflect.ValueOf(f1)})
		return
	}
	if rVal.CanAddr() {
		rVal.Set(reflect.ValueOf(f1))
		return
	}
	fmt.Printf("%v %v: %v\n", rPtrType, methodName, err)
	return

}

func (w *WindowModel) ShowColor3(rPtrType reflect.Type, rPtrVal reflect.Value, fieldName string) {
	rVal := rPtrVal.Elem().FieldByName(fieldName)
	f3 := rVal.Interface().(mgl32.Vec3)
	flags := imgui.ColorEditFlagsFloat | imgui.ColorEditFlagsAlphaPreviewHalf | imgui.ColorEditFlagsAlphaBar
	imgui.ColorEdit3V(fmt.Sprintf("##%s", fieldName), (*[3]float32)(&f3), flags)

	if rVal.CanAddr() {
		rVal.Set(reflect.ValueOf(f3))
	}
}

func (w *WindowModel) SetRenderObj(renderObj interface{}) {
	w.modelObj = renderObj
}

func (w *WindowModel) SetVisible(visible bool) {
	w.visible = visible
}
