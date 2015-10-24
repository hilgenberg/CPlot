#include "Graph.h"
#include "Plot.h"
#include "OpenGL/GL_Color.h"
#include "OpenGL/GL_Font.h"
#include "../Utility/System.h"
#include "../Engine/Namespace/RootNamespace.h"
#include <cassert>
#include <algorithm>
#include <string>
#include <stdexcept>
#include <cassert>

const Namespace &Graph::pns() const{ return plot.ns; }

//--------------------------------------------------------------------------------------------

static const std::vector<std::string> &image_names()
{
	static std::vector<std::string> names;
	if (names.empty())
	{
		names.push_back("custom");
		names.push_back("colors");
		names.push_back("xor");
		names.push_back("grey_xor");
		names.push_back("checker");
		names.push_back("plasma");
		names.push_back("colors2");
		names.push_back("phase");
		names.push_back("units");
		assert(names.size() == MAX_VALID_IMAGE_PATTERN_ID+1);
	}
	return names;
}
static std::string to_string(const GL_Image &im)
{
	auto &n = image_names();
	int i = (int)im.is_pattern();
	// TODO: remember source path
	if (i == IP_CUSTOM) return format("custom (%d x %d)", im.w(), im.h());

	if (i < (int)n.size()) return n[i];
	assert(false);
	return "corrupt";
}
static void load_image(GL_Image &im, const std::string &s)
{
	auto &n = image_names();
	for (int i = 1; i < (int)n.size(); ++i)
	{
		if (s == n[i]){ im = (GL_ImagePattern)i; return; }
	}
	// TODO: load image file
	throw error("Not a valid image pattern name", s);
}

//--------------------------------------------------------------------------------------------

static const std::vector<std::string> &mask_names()
{
	static std::vector<std::string> names;
	if (names.empty())
	{
		names.push_back("off");
		names.push_back("circles");
		names.push_back("squares");
		names.push_back("triangles");
		names.push_back("rounded_rect");
		names.push_back("chessboard");
		names.push_back("hlines");
		names.push_back("vlines");
		names.push_back("rings");
		names.push_back("static");
		names.push_back("fan");
		names.push_back("hexagon");
	}
	return names;
}

//--------------------------------------------------------------------------------------------

void Graph::init_properties()
{
	auto RECALC = [this]{ update(CH_UNKNOWN); };

	//-----------------------------------------------------------------------------------
	// GraphOptions
	//-----------------------------------------------------------------------------------

	Property &h = props["hidden"];
	h.desc = "hidden state";
	h.get  = [this]{ return format_bool(options.hidden); };
	h.set  = [this](const std::string &s){ options.hidden = parse_bool(s); plot.update_axis(); };
	h.type = PT_Bool;

	Property &fill = props["fill"];
	fill.desc = "fill color";
	fill.vis  = [this]{ return hasFill(); };
	fill.get  = [this]{ return options.fill_color.to_string(); };
	fill.set  = [this](const std::string &s){ options.fill_color = s; };
	fill.type = PT_Color;

	Property &line = props["line"];
	line.desc = "line color";
	line.vis  = [this]{ return usesLineColor(); };
	line.get  = [this]{ return options.line_color.to_string(); };
	line.set  = [this](const std::string &s){ options.line_color = s; };
	line.type = PT_Color;

	Property &grid = props["grid"];
	grid.desc = "grid color";
	grid.vis  = [this]{ return !isVectorField() && !isColor() && hasFill() && options.grid_style != Grid_Off; };
	grid.get  = [this]{ return options.grid_color.to_string(); };
	grid.set  = [this](const std::string &s){ options.grid_color = s; };
	grid.type = PT_Color;

	Property &clip = props["clip"];
	clip.desc = "clip to axis";
	clip.vis  = [this]{ return plot.axis.type() == Axis::Box; };
	clip.get  = [this]{ return format_bool(clipping()); };
	clip.set  = [this](const std::string &s){ clipping(parse_bool(s)); update(CH_UNKNOWN); };
	clip.type = PT_Bool;

	Property &dd = props["disco"];
	dd.desc = "discontinuity detection";
	dd.vis  = [this]{ return !isVectorField() && !isColor(); };
	dd.get  = [this]{ return format_bool(options.disco); };
	dd.set  = [this](const std::string &s){ options.disco = parse_bool(s); update(CH_UNKNOWN); };
	dd.type = PT_Bool;

	Property &tex = props["tex"];
	tex.desc = "texture opacity";
	//TODO tex
	tex.get = [this]()->std::string{ return format_percentage(options.texture_opacity); };
	tex.set = [this](const std::string &s){ options.texture_opacity = parse_percentage(s); };
	
	Property &rtex = props["rtex"];
	rtex.desc = "reflection texture opacity";
	//TODO rtex
	rtex.get = [this]()->std::string{ return format_percentage(options.reflection_opacity); };
	rtex.set = [this](const std::string &s){ options.reflection_opacity = parse_percentage(s); };
	
	Property &shine = props["shine"];
	shine.desc = "surface reflectiveness";
	//TODO vis
	shine.get = [this]()->std::string{ return format_percentage(options.shinyness); };
	shine.set = [this](const std::string &s){ options.shinyness = parse_percentage(s); };
	
	Property &lw = props["lw"];
	lw.desc = "line width";
	//TODO vis
	lw.get = [this]()->std::string{ return format_double(options.line_width); };
	lw.set = [this](const std::string &s){ options.line_width = parse_double(s); };
	
	Property &glw = props["glw"];
	glw.desc = "gridline width";
	//TODO vis
	glw.get = [this]()->std::string{ return format_double(options.gridline_width); };
	glw.set = [this](const std::string &s){ options.gridline_width = parse_double(s); };

	Property &gd = props["gd"];
	gd.desc = "grid density";
	//TODO vis
	gd.get = [this]()->std::string{ return format_double(options.grid_density); };
	gd.set = [this](const std::string &s){ options.grid_density = parse_double(s); update(CH_UNKNOWN); };
	
	Property &vfs = props["vfs"];
	vfs.desc = "vector field scale";
	//TODO vis
	vfs.get = [this]()->std::string{ return format_double(options.vf_scale); };
	vfs.set = [this](const std::string &s){ options.vf_scale = parse_double(s); };

	Property &hs = props["hs"];
	hs.desc = "histogram scale";
	//TODO vis
	hs.get = [this]()->std::string{ return format_double(options.hist_scale); };
	hs.set = [this](const std::string &s){ options.hist_scale = parse_double(s); };

	Property &q = props["q"];
	q.desc = "quality";
	//TODO vis
	q.get = [this]()->std::string{ return format_percentage(options.quality); };
	q.set = [this](const std::string &s){ options.quality = parse_percentage(s); update(CH_UNKNOWN); };

	Property &sm = props["sm"];
	sm.desc = "shading mode";
	//TODO vis
	assert(sizeof(int) == sizeof(ShadingMode));
	sm.set_enum(RECALC, (int&)options.shading_mode, 
		"points",     Shading_Points,
		"wireframe",  Shading_Wireframe,
		"hiddenline", Shading_Hiddenline,
		"flat",       Shading_Flat,
		"smooth",     Shading_Smooth,
		NULL);

	Property &hm = props["hm"];
	hm.desc = "histogram mode";
	//TODO vis
	assert(sizeof(int) == sizeof(HistogramMode));
	hm.set_enum(RECALC, (int&)options.hist_mode, 
		"riemann", HM_Riemann,
		"disc",    HM_Disc,
		"normal",  HM_Normal,
		NULL);

	Property &vm = props["vm"];
	vm.desc = "vector field mode";
	//TODO vis
	assert(sizeof(int) == sizeof(VectorfieldMode));
	vm.set_enum(RECALC, (int&)options.vf_mode, 
		"unscaled",   VF_Unscaled,
		"normalized", VF_Normalized,
		"direction",  VF_Unit,
		"flow",       VF_Connected,
		NULL);

	Property &tm = props["tm"];
	tm.desc = "texture mode";
	//TODO vis
	assert(sizeof(int) == sizeof(TextureProjection));
	tm.set_enum(RECALC, (int&)options.vf_mode, 
		"tile",    TP_Repeat,
		"center",  TP_Center,
		"riemann", TP_Riemann,
		"uv",      TP_UV,
		NULL);

	Property &bm = props["bm"];
	bm.desc = "blend mode";
	bm.get  = [this]
	{
		auto &m0 = options.transparency;
		for (auto &m : DefaultBlendModes()) if (m0 == m.mode) return m.name;
		return std::string("custom");
	};
	bm.set  = [this](const std::string &s)
	{
		for (auto &m : DefaultBlendModes())
			if (m.name == s){ options.transparency = m.mode; return; }
		throw error("Not a valid blend mode", s);
	};
	//TODO vis
	bm.values = []{
		std::vector<std::string> V;
		for (auto &m : DefaultBlendModes()) V.push_back(m.name);
		return V;
	};

	Property &tim = props["tim"];
	tim.desc = "texture image";
	//TODO vis
	tim.get = [this]{ return to_string(options.texture); };
	tim.set = [this](const std::string &s){ load_image(options.texture, s); };
	tim.values = []{ return image_names(); };

	Property &rim = props["rim"];
	rim.desc = "reflection image";
	//TODO vis
	rim.get = [this]{ return to_string(options.reflection_texture); };
	rim.set = [this](const std::string &s){ load_image(options.reflection_texture, s); };
	rim.values = tim.values;

	Property &mask = props["mask"];
	mask.desc = "alpha mask";
	//TODO vis
	mask.get = [this]
	{
		GL_Mask &m = options.mask;
		auto &n = mask_names();
		MaskStyle s = m.style();
		if (s == Mask_Custom) return format("custom:%g", m.density());
		if (s == Mask_Off) return std::string("off");
		if ((int)s < (int)n.size()) return format("%s:%g", n[(int)s].c_str(), m.density());
		assert(false);
		return std::string("corrupt");
	};
	mask.set = [this](const std::string &s)
	{
		GL_Mask &m = options.mask;
		auto &n = mask_names();
		// syntax: <name>:<d> | <name>[:] | :<d>
		auto i = s.find(':');
		std::string name;
		double d = m.density();
		if (i == 0)
		{
			m.density(parse_double(s.substr(1)));
			return;
		}
		else if (i+1 == s.length())
		{
			name = s.substr(0, s.length()-1);
		}
		else if (i != std::string::npos)
		{
			name = s.substr(0,i);
			d = parse_double(s.substr(i+1));
		}
		else
		{
			name = s;
		}
		for (int i = 0; i < (int)n.size(); ++i)
		{
			if (name != n[i]) continue;
			m.style((MaskStyle)i);
			m.density(d);
			return;
		}
		throw error("Not a valid alpha mask", s);
	};
	mask.values = []{ return mask_names(); };
}
