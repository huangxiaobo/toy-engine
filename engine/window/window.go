package window

import (
	"github.com/inkyblackness/imgui-go/v4"
)

type WindowFlags struct {
	noTitlebar     bool
	noScrollbar    bool
	noMenu         bool
	noMove         bool
	noResize       bool
	noCollapse     bool
	noNav          bool
	noBackground   bool
	noBringToFront bool
}

func (f WindowFlags) combined() imgui.WindowFlags {
	flags := imgui.WindowFlagsNone
	if f.noTitlebar {
		flags |= imgui.WindowFlagsNoTitleBar
	}
	if f.noScrollbar {
		flags |= imgui.WindowFlagsNoScrollbar
	}
	if !f.noMenu {
		flags |= imgui.WindowFlagsMenuBar
	}
	if f.noMove {
		flags |= imgui.WindowFlagsNoMove
	}
	if f.noResize {
		flags |= imgui.WindowFlagsNoResize
	}
	if f.noCollapse {
		flags |= imgui.WindowFlagsNoCollapse
	}
	if f.noNav {
		flags |= imgui.WindowFlagsNoNav
	}
	if f.noBackground {
		flags |= imgui.WindowFlagsNoBackground
	}
	if f.noBringToFront {
		flags |= imgui.WindowFlagsNoBringToFrontOnFocus
	}
	return flags
}
