#pragma once

#include <string>
#include <sstream>
#include <iostream>
#include "singleton.h"
#include "imgui/imgui.h"


inline namespace Configuration
{
	class Settings : public Singleton<Settings>
	{
	public:

		const char* BoxTypes[2] = { "Full Box","Cornered Box" };
		const char* LineTypes[3] = { "Bottom To Enemy","Top To Enemy","Crosshair To Enemy" };


		bool b_MenuShow = false;


		bool b_Visual = false;
		bool b_EspBox = false;
		bool b_EspSkeleton = false;
		bool b_EspLine = false;
		bool b_EspDistance = false;
		bool b_EspHealth  = false;
		bool b_EspName = false;

		ImColor BoxColor = ImColor(255.f / 255, 0.f, 0.f);
		float fl_BoxColor[3] = { 255.f / 255,0.f,0.f};//
		ImColor SkeletonColor = ImColor(255.f / 255, 255.f / 255, 255.f / 255);
		float fl_SkeletonColor[3] = { 255.f / 255,255.f / 255,255.f / 255};//
		ImColor LineColor = ImColor(0.f, 0.f, 255.f / 255);
		float fl_LineColor[3] = { 0 ,0, 255.f / 255};  //


		int in_BoxType = 0;
		int in_LineType = 0;
		int in_tab_index = 0;


	};
#define CFG Configuration::Settings::Get()
}