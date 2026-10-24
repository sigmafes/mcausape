#include "OptionsGroup.h"
#include "../../Minecraft.h"
#include "ImageButton.h"
#include "OptionsItem.h"
#include "Slider.h"
#include "../../../locale/I18n.h"
#include "Button.h"
#include "../../../platform/input/Mouse.h"

/** The shader quality, beside the shaders' switch: "Low" or "High", shown
    only while the shaders are on (one switch, not two). */
class ShaderQualityButton: public Touch::TButton
{
	typedef Touch::TButton super;
public:
	ShaderQualityButton() : super(9999998, 0, 0, 34, 14, "Low") {}
	void render(Minecraft* minecraft, int xm, int ym) {
		if (!minecraft->options.shaders) return;
		msg = minecraft->options.shaderHigh ? "High" : "Low";
		super::render(minecraft, xm, ym);
	}
	void mouseClicked(Minecraft* minecraft, int x, int y, int buttonNum) {
		if (!minecraft->options.shaders || buttonNum != MouseAction::ACTION_LEFT) return;
		if (clicked(minecraft, x, y)) minecraft->options.toggle(&Options::Option::SHADER_HIGH, 1);
	}
};

/// The switch and the quality button side by side, laid out each frame
/// from where OptionsItem puts the row.
class ShaderRow: public GuiElementContainer
{
	typedef GuiElementContainer super;
public:
	ShaderRow(GuiElement* toggle, GuiElement* quality)
	: super(false, true, 0, 0, quality->width + 6 + toggle->width, toggle->height), toggle(toggle), quality(quality) {
		addChild(toggle); addChild(quality);
	}
	void layout() {
		toggle->x = x; toggle->y = y;
		quality->x = x + toggle->width + 6; quality->y = y + (toggle->height - quality->height) / 2;
	}
	void render(Minecraft* minecraft, int xm, int ym) { layout(); super::render(minecraft, xm, ym); }
	void mouseClicked(Minecraft* minecraft, int x, int y, int buttonNum) { layout(); super::mouseClicked(minecraft, x, y, buttonNum); }
private:
	GuiElement* toggle; GuiElement* quality;
};

OptionsGroup& OptionsGroup::addShaderRow(Minecraft* minecraft) {
	ImageDef def;
	def.setSrc(IntRectangle(160, 206, 39, 20));
	def.name = "gui/touchgui.png";
	def.width = 39 * 0.7f;
	def.height = 20 * 0.7f;
	OptionButton* toggle = new OptionButton(&Options::Option::SHADERS);
	toggle->setImageDef(def, true);
	toggle->updateImage(&minecraft->options);
	OptionsItem* item = new OptionsItem(I18n::get("options.shaders"), new ShaderRow(toggle, new ShaderQualityButton()));
	addChild(item);
	setupPositions();
	return *this;
}
OptionsGroup::OptionsGroup( std::string labelID )  {
	label = I18n::get(labelID);
}

void OptionsGroup::setupPositions() {
	// First we write the header and then we add the items
	int curY = y + 10;
	for(std::vector<GuiElement*>::iterator it = children.begin(); it != children.end(); ++it) {
		(*it)->width = width - 5;

		(*it)->y = curY;
		(*it)->x = x + 10;
		(*it)->setupPositions();
		curY += (*it)->height + 3;
	}
	// A height, not a bottom edge -- OptionsPane stacks groups by adding this
	// to a running y, so leaving it absolute pushed every group after the
	// first off the bottom of the pane.
	height = curY - y;
}

void OptionsGroup::render( Minecraft* minecraft, int xm, int ym ) {
	minecraft->font->draw(label, (float)x + 2, (float)y, 0xffffffff, false);
	super::render(minecraft, xm, ym);
}

OptionsGroup& OptionsGroup::addOptionItem( const Options::Option* option, Minecraft* minecraft ) {
	if(option->isBoolean())
		createToggle(option, minecraft);
	else if(option->isProgress())
		createProgressSlider(option, minecraft);
	else if(option->isInt())
		createStepSlider(option, minecraft);
	return *this;
}

void OptionsGroup::createToggle( const Options::Option* option, Minecraft* minecraft ) {
	ImageDef def;
	def.setSrc(IntRectangle(160, 206, 39, 20));
	def.name = "gui/touchgui.png";
	def.width = 39 * 0.7f;
	def.height = 20 * 0.7f;
	OptionButton* element = new OptionButton(option);
	element->setImageDef(def, true);
	// Without this the button picks its on/off frame from an uninitialised
	// flag, so it can open showing the opposite of the actual setting.
	element->updateImage(&minecraft->options);
	std::string itemLabel = I18n::get(option->getCaptionId());
	OptionsItem* item = new OptionsItem(itemLabel, element);
	addChild(item);
	setupPositions();
}

void OptionsGroup::createProgressSlider( const Options::Option* option, Minecraft* minecraft ) {
	Slider* element = new Slider(minecraft,
									option,
									minecraft->options.getProgrssMin(option),
									minecraft->options.getProgrssMax(option));
	element->width = 100;
	element->height = 20;
	// The row is named after the option, not after the group it sits in --
	// using `label` here is what made the sensitivity row read
	// "options.group.mojang<" instead of "Sensitivity".
	OptionsItem* item = new OptionsItem(I18n::get(option->getCaptionId()), element);
	addChild(item);
	setupPositions();
}

void OptionsGroup::createStepSlider( const Options::Option* option, Minecraft* minecraft ) {
	std::vector<int> steps = minecraft->options.getSteps(option);
	if (steps.size() < 2) return;

	Slider* element = new Slider(minecraft, option, steps);
	element->width = 100;
	element->height = 20;
	OptionsItem* item = new OptionsItem(I18n::get(option->getCaptionId()), element);
	addChild(item);
	setupPositions();
}
