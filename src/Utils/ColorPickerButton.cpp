#include "ColorPickerButton.h"

#pragma GCC diagnostic ignored "-Wformat-security"

using namespace ColorButton;

// ColorButton::Button::Button(const char* popupname, ImColor &color){
// 	if ( ImGui::ColorButton(XORSTR(popupname), (ImVec4)Settings::ESP::enemyVisibleColor.color, 0, ImVec2(20,20)) ){
// 		float col[] =
// 		{
// 			Settings::ESP::enemyVisibleColor.color.Value.x, 
// 			Settings::ESP::enemyVisibleColor.color.Value.y, 
// 			Settings::ESP::enemyVisibleColor.color.Value.z, 
// 			Settings::ESP::enemyVisibleColor.color.Value.w
// 		};
// 		ImGui::ColorPicker4(XORSTR(popupname), col);
// 	}
// }


// void ColorButton::RenderWindow(const char* popupname, const int& ColorIndex,const bool show)
// {
//     // if ( ImGui::ColorButton(XORSTR("##ColorPicker"), colors[ColorIndex].ColorVec4, ImGuiColorEditFlags_RGB, ImVec2(20,20)) )
//     // 	ColorButton::showWindow = !ColorButton::showWindow;
// 	if (show)
//    	 	ImGui::OpenPopup(popupname);

//     ImGui::SetNextWindowPos(ImVec2(ImGui::GetMousePos().x, ImGui::GetMousePos().y), ImGuiCond_Once);
//     ImGui::SetNextWindowSize(ImVec2(240, 150), ImGuiCond_Once);
// 	if ( ImGui::BeginPopup(popupname, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove) )
//     {
// 		Settings::UI::Windows::Colors::open = true;
//     	ImVec2 temp = ImGui::GetWindowSize();
//     	Settings::UI::Windows::Colors::sizeX = (int)temp.x;
//     	Settings::UI::Windows::Colors::sizeY = (int)temp.y;
//     	temp = ImGui::GetWindowPos();
//     	Settings::UI::Windows::Colors::posX = (int)temp.x;
//     	Settings::UI::Windows::Colors::posY = (int)temp.y;

//     	if (colors[ColorIndex].type == ColorListVar::HEALTHCOLORVAR_TYPE)
//     	{
// 			UI::ColorPicker4((float*)colors[ColorIndex].healthColorVarPtr);
// 			ImGui::Checkbox(XORSTR("Rainbow"), &colors[ColorIndex].healthColorVarPtr->rainbow);
// 			ImGui::SameLine();
// 			ImGui::Checkbox(XORSTR("Health-Based"), &colors[ColorIndex].healthColorVarPtr->hp);
// 			ImGui::SliderFloat(XORSTR("##RAINBOWSPEED"), &colors[ColorIndex].healthColorVarPtr->rainbowSpeed, 0.f, 1.f, "Rainbow Speed: %0.3f");
//     	}
//     	else
//     	{
// 			UI::ColorPicker4((float*)colors[ColorIndex].colorVarPtr);
// 			ImGui::Checkbox(XORSTR("Rainbow"), &colors[ColorIndex].colorVarPtr->rainbow);
// 			ImGui::PushItemWidth(-1);
// 			ImGui::SliderFloat(XORSTR("##RAINBOWSPEED"), &colors[ColorIndex].colorVarPtr->rainbowSpeed, 0.f, 1.f, "Rainbow Speed: %0.3f");
// 			ImGui::PopItemWidth();
//     	}
// 		ImGui::EndPopup();
// 	}

// ShowColorWindow(ColorIndex);
// }
