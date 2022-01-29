package utils

import (
	"fmt"
	"github.com/go-gl/gl/v4.1-core/gl"
	"image"
	"image/color"
	"image/png"
	"log"
	"os"
)

func Screenshot(width, height int) {
	fmt.Printf("ScreenCat, %v %v\n", width, height)
	//创建一块内存
	pixes := make([]uint8, width*height*4+1)

	//glReadBuffer函数指明要从哪个颜色缓存中读取数据
	gl.ReadBuffer(gl.FRONT)
	//glReadPixels 做了实际的读取工作
	gl.ReadPixels(0, 0, int32(width), int32(height), gl.RGBA, gl.UNSIGNED_BYTE, gl.Ptr(&pixes[0]))
	gl.ReadBuffer(gl.NONE)

	// Save that RGBA image to disk.
	outFile, err := os.Create("./output/out.png")
	if err != nil {
		log.Println(err)
		os.Exit(1)
	}
	defer outFile.Close()

	idx := 0
	myImage := image.NewRGBA(image.Rect(0, 0, width, height))
	for y := height - 1; y >= 0; y-- {
		for x := 0; x < width; x++ {
			r := pixes[idx+0]
			g := pixes[idx+1]
			b := pixes[idx+2]

			myImage.Set(x, y, color.RGBA{R: r, B: b, G: g, A: 255})
			idx += 4
		}
	}

	png.Encode(outFile, myImage)

	if err != nil {
		log.Println(err)
		os.Exit(1)
	}

	fmt.Printf("ScreenCat, end\n")

}
