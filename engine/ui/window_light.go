package ui

import (
	"fmt"
	"github.com/go-gl/mathgl/mgl32"
	"github.com/inkyblackness/imgui-go/v4"
	"reflect"
)

type WindowLight struct {
	noClose bool
	visible bool
	flags   WindowFlags

	lightObj interface{}
}

func NewWindowLight() *WindowLight {
	w := &WindowLight{
		visible:  false,
		flags:    WindowFlags{noResize: true, noMenu: true, noTitlebar: true, noCollapse: true, noBackground: false},
		lightObj: nil,
	}

	return w
}

func (w *WindowLight) Reset() {
	w.lightObj = nil
}

//var colorUI = [3]float32{-1, -1, -1}

func (w *WindowLight) Show(displaySize [2]float32) {
	if !w.visible || w.lightObj == nil {
		return
	}
	imgui.SetNextWindowPosV(imgui.Vec2{X: displaySize[0] - 320, Y: 0}, imgui.ConditionNone, imgui.Vec2{})
	imgui.SetNextWindowSizeV(imgui.Vec2{X: 320, Y: displaySize[1]}, imgui.ConditionNone)

	defer imgui.End()
	if !imgui.BeginV("LightPanel", &w.visible, w.flags.combined()) {
		return
	}

	imgui.PushItemWidth(imgui.FontSize() * -12)

	rPtrType := reflect.TypeOf(w.lightObj)
	rPtrVal := reflect.ValueOf(w.lightObj)

	//rType := rPtrType
	//rVal := rPtrVal
	//if rPtrType.Kind() == reflect.Ptr {
	//	rType = rPtrType.Elem()
	//	rVal = rPtrVal.Elem()
	//}

	imgui.Spacing()
	imgui.Spacing()
	imgui.Bullet()
	imgui.Text("Geometry")
	imgui.Indent()

	w.ShowFloat4(rPtrType, rPtrVal, "Position")

	imgui.Unindent()
	imgui.Spacing()
	imgui.Spacing()
	imgui.Bullet()
	imgui.Text("Color")
	imgui.Indent()

	w.ShowColor3(rPtrType, rPtrVal, "Color")
	w.ShowFloat(rPtrType, rPtrVal, "AmbientIntensity")
	w.ShowFloat(rPtrType, rPtrVal, "DiffuseIntensity")
	w.ShowColor3(rPtrType, rPtrVal, "DiffuseColor")
	w.ShowColor3(rPtrType, rPtrVal, "SpecularColor")

	imgui.Unindent()
	imgui.Spacing()
	imgui.Spacing()
	imgui.Bullet()
	imgui.Text("Atten")
	imgui.Indent()
	atten := rPtrVal.Elem().FieldByName("Atten").Interface()
	attenPtrType := reflect.TypeOf(atten)
	attenPtrVal := reflect.ValueOf(atten)
	w.ShowFloat(attenPtrType, attenPtrVal, "Constant")
	w.ShowFloat(attenPtrType, attenPtrVal, "Linear")
	w.ShowFloat(attenPtrType, attenPtrVal, "Exp")
	imgui.Unindent()

	imgui.Unindent()
}

func (w *WindowLight) ShowFloat3(rPtrType reflect.Type, rPtrVal reflect.Value, fieldName string) {
	position := rPtrVal.Elem().FieldByName(fieldName)
	f3 := position.Interface().(mgl32.Vec3)

	imgui.DragFloat3(fieldName, (*[3]float32)(&f3))

	methodName := fmt.Sprintf("Set%s", fieldName)
	method, err := rPtrType.MethodByName(methodName)
	if err != true {
		fmt.Printf("%v %v: %v\n", rPtrType, methodName, err)
		return
	}
	method.Func.Call([]reflect.Value{rPtrVal, reflect.ValueOf(f3)})
}

func (w *WindowLight) ShowFloat4(rPtrType reflect.Type, rPtrVal reflect.Value, fieldName string) {
	position := rPtrVal.Elem().FieldByName(fieldName)
	f4 := position.Interface().(mgl32.Vec4)

	imgui.DragFloat4(fieldName, (*[4]float32)(&f4))
	f4[3] = 1

	methodName := fmt.Sprintf("Set%s", fieldName)
	method, err := rPtrType.MethodByName(methodName)
	if err != true {
		fmt.Printf("%v %v: %v\n", rPtrType, methodName, err)
		return
	}
	//fmt.Printf("method: %v, %v\n", method, colorUI)
	method.Func.Call([]reflect.Value{rPtrVal, reflect.ValueOf(f4)})
}

func (w *WindowLight) ShowFloat(rPtrType reflect.Type, rPtrVal reflect.Value, fieldName string) {
	var position reflect.Value
	if rPtrType.Kind() == reflect.Ptr {
		position = rPtrVal.Elem().FieldByName(fieldName)
	} else {
		position = rPtrVal.FieldByName(fieldName)
	}

	f1 := position.Interface().(float32)
	imgui.DragFloat(fieldName, &f1)

	methodName := fmt.Sprintf("Set%s", fieldName)
	method, err := rPtrType.MethodByName(methodName)
	if err != true {
		if rPtrType.Kind() == reflect.Ptr {
			rPtrVal.Elem().FieldByName(fieldName).Set(reflect.ValueOf(f1))
		} else {
			rPtrVal.FieldByName(fieldName).Set(reflect.ValueOf(f1))
		}
	} else {
		method.Func.Call([]reflect.Value{rPtrVal, reflect.ValueOf(f1)})
	}
}

func (w *WindowLight) ShowColor3(rPtrType reflect.Type, rPtrVal reflect.Value, fieldName string) {
	rVal := rPtrVal.Elem().FieldByName(fieldName)
	f3 := rVal.Interface().(mgl32.Vec3)
	imgui.ColorEdit3(fieldName, (*[3]float32)(&f3))

	if rVal.CanAddr() {
		rVal.Set(reflect.ValueOf(f3))
	}
}

func (w *WindowLight) SetLight(lightObj interface{}) {
	w.lightObj = lightObj
}

func (w *WindowLight) SetVisible(visible bool) {
	w.visible = visible
}
