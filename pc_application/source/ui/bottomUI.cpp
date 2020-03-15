#include "bottomUI.hpp"

ButtonGrid::ButtonGrid(wxFrame* parent, wxSize requiredSize, std::shared_ptr<ButtonData> data, DataProcessing* inputs)
	: DrawingCanvas(parent, requiredSize) {
	SetDoubleBuffered(false);

	buttonData             = data;
	totalCombinedImageSize = requiredSize;
	inputInstance          = inputs;

	setBackgroundColor(*wxWHITE);

	// Handle grid clicking
	Bind(wxEVT_LEFT_DOWN, &ButtonGrid::onGridClick, this);

	for(auto const& button : buttonData->buttonMapping) {
		char buf[5];
		sprintf(buf, "%d-%d", button.second->gridX, button.second->gridY);
		locToButton[std::string(buf)] = button.first;
	}
}

void ButtonGrid::draw(wxDC& dc) {
	int width;
	int height;
	GetSize(&width, &height);

	// Set scaling for the image to render without wxImage
	double scaleWidth  = (double)width / totalCombinedImageSize.GetWidth();
	double scaleHeight = (double)height / totalCombinedImageSize.GetHeight();
	dc.SetUserScale(scaleWidth, scaleHeight);

	// TODO not every image is a square :)
	for(auto const& button : buttonData->buttonMapping) {
		wxBitmap* bitmap;
		if(inputInstance->getButtonState(button.first)) {
			bitmap = button.second->resizedGridOnBitmap;
		} else {
			bitmap = button.second->resizedGridOffBitmap;
		}
		wxPoint location;
		location.x = button.second->gridX * bitmap->GetWidth();
		location.y = button.second->gridY * bitmap->GetHeight();

		dc.DrawBitmap(*bitmap, location, true);
	}
}

void ButtonGrid::onGridClick(wxMouseEvent& event) {
	wxPoint windowCoord = event.GetPosition();
	// Divide to handle scaling
	int width;
	int height;
	GetSize(&width, &height);

	// Want to remove everything after the decimal point
	int X = (int)(((float)windowCoord.x / width) * buttonData->KeyWidth);
	int Y = (int)(((float)windowCoord.y / height) * buttonData->KeyHeight);

	char buf[5];
	sprintf(buf, "%d-%d", X, Y);
	std::string key(buf);

	if(locToButton.count(key)) {
		// Element exists, time to trigger the click
		inputInstance->handleButtonInput(locToButton[key]);
		Refresh();
	}
}

FrameViewerCanvas::FrameViewerCanvas(wxFrame* parent, wxBitmap* defaultImage)
	: DrawingCanvas(parent, wxSize(1280, 720)) {
	// Needs to be able to fit the frames
	hasFrameToRender  = false;
	defaultBackground = defaultImage;
}

void FrameViewerCanvas::draw(wxDC& dc) {
	int width;
	int height;
	GetSize(&width, &height);
	if(!hasFrameToRender) {
		// Set scaling for the image to render without wxImage
		dc.SetUserScale((double)width / defaultBackground->GetWidth(), (double)height / defaultBackground->GetHeight());
		// Render the default image, that's it
		dc.DrawBitmap(*defaultBackground, 0, 0, false);
	} else {
		// Do thing
	}
}

JoystickCanvas::JoystickCanvas(wxFrame* parent, DataProcessing* inputData, uint8_t leftJoy)
	: DrawingCanvas(parent, wxSize(150, 150)) {
	// Should be a decent size
	isLeftJoystick = leftJoy;
	inputInstance  = inputData;

	// Create widgets
	xInput                     = new wxSpinCtrl(parent);
	yInput                     = new wxSpinCtrl(parent);
	canGoOutsideCircleCheckbox = new wxCheckBox(parent, wxID_ANY, wxEmptyString);

	xInput->Bind(wxEVT_COMMAND_SPINCTRL_UPDATED, &JoystickCanvas::xValueSet, this);
	yInput->Bind(wxEVT_COMMAND_SPINCTRL_UPDATED, &JoystickCanvas::yValueSet, this);

	Bind(wxEVT_LEFT_DOWN, &JoystickCanvas::onMouseClick, this);
	Bind(wxEVT_MOTION, &JoystickCanvas::onMouseDrag, this);

	xInput->SetMin(ButtonData::axisMin);
	xInput->SetMax(ButtonData::axisMax);
	yInput->SetMin(ButtonData::axisMin);
	yInput->SetMax(ButtonData::axisMax);

	inputSizer  = new wxBoxSizer(wxHORIZONTAL);
	widgetSizer = new wxBoxSizer(wxVERTICAL);

	inputSizer->Add(xInput, 0, wxEXPAND);
	inputSizer->Add(yInput, 0, wxEXPAND);
	inputSizer->Add(canGoOutsideCircleCheckbox, 0, wxEXPAND);

	widgetSizer->Add(this, 0, wxSHAPED | wxEXPAND);
	widgetSizer->Add(inputSizer, 1, wxEXPAND | wxALL);
}

void JoystickCanvas::draw(wxDC& dc) {
	// Do thing
	int width;
	int height;
	GetSize(&width, &height);

	wxPoint approximateMiddle((float)width / 2, (float)height / 2);

	dc.SetPen(*wxGREEN_PEN);
	dc.SetBrush(*wxLIGHT_GREY_BRUSH);

	dc.DrawCircle(approximateMiddle, approximateMiddle.x);

	int32_t joyX;
	int32_t joyY;

	if(isLeftJoystick) {
		joyX = inputInstance->getCurrentFrame()->LS_X;
		joyY = inputInstance->getCurrentFrame()->LS_Y;
	} else {
		joyX = inputInstance->getCurrentFrame()->RS_X;
		joyY = inputInstance->getCurrentFrame()->RS_Y;
	}

	// Flip the height to resemble the coordinate system
	joyY *= -1;

	int renderJoyX = ((float)joyX / ButtonData::axisMax) * approximateMiddle.x + approximateMiddle.x;
	int renderJoyY = ((float)joyY / ButtonData::axisMax) * approximateMiddle.y + approximateMiddle.y;

	int middleCircleRadius = std::floor((float)width / 20);

	// Optimise if the angle is nonexistent
	if(joyX != 0 || joyY != 0) {
		float joyAngle = std::atan2(joyY, joyX);

		// First, draw the rectangle
		dc.SetBrush(*wxMEDIUM_GREY_BRUSH);

		wxPoint joystickBarPoints[4];

		// Calculate the angle once and then mirror to use twice
		float barDrawingAngle = HELPERS::normalizeRadian(joyAngle + (M_PI / 2.0f));
		int offsetX           = middleCircleRadius * std::cos(barDrawingAngle);
		int offsetY           = middleCircleRadius * std::sin(barDrawingAngle);

		joystickBarPoints[0] = wxPoint(approximateMiddle.x + offsetX, approximateMiddle.y + offsetY);
		joystickBarPoints[1] = wxPoint(renderJoyX + offsetX, renderJoyY + offsetY);
		joystickBarPoints[2] = wxPoint(renderJoyX - offsetX, renderJoyY - offsetY);
		joystickBarPoints[3] = wxPoint(approximateMiddle.y - offsetX, approximateMiddle.y - offsetY);

		dc.DrawPolygon(4, joystickBarPoints);
	}

	// Draw the middle ball
	dc.SetBrush(*wxBLACK_BRUSH);
	dc.DrawCircle(approximateMiddle, middleCircleRadius);

	// Finally, draw the joystick itself
	dc.DrawCircle(renderJoyX, renderJoyY, middleCircleRadius + 5);

	// Show the actual value underneath
	if(isLeftJoystick) {
		xInput->SetValue(inputInstance->getCurrentFrame()->LS_X);
		yInput->SetValue(inputInstance->getCurrentFrame()->LS_Y);
	} else {
		xInput->SetValue(inputInstance->getCurrentFrame()->RS_X);
		yInput->SetValue(inputInstance->getCurrentFrame()->RS_Y);
	}
}

void JoystickCanvas::correctForCircleLock() {
	if(canGoOutsideCircleCheckbox->IsChecked()) {
		int* x;
		int* y;
		if(isLeftJoystick) {
			x = &(inputInstance->getCurrentFrame()->LS_X);
			y = &(inputInstance->getCurrentFrame()->LS_Y);
		} else {
			x = &(inputInstance->getCurrentFrame()->RS_X);
			y = &(inputInstance->getCurrentFrame()->RS_Y);
		}
		// This corrects for circle lock if the checkbox is set
		// https://math.stackexchange.com/a/127615
		int radiusSquared = std::pow(*x, 2) + std::pow(*y, 2);
		if(radiusSquared > std::pow(ButtonData::axisMax, 2)) {
			// Have to clamp it
			*x = ButtonData::axisMax * (*x / std::sqrt(radiusSquared));
			*y = ButtonData::axisMax * (*y / std::sqrt(radiusSquared));
		}
	}
}

void JoystickCanvas::onMouseClick(wxMouseEvent& event) {
	wxPoint loc = event.GetPosition();
	int width;
	int height;
	GetSize(&width, &height);

	int32_t scaledX = ((float)loc.x / width) * 60000 - ButtonData::axisMax;
	// Y is flipped
	int32_t scaledY = (((float)loc.y / height) * 60000 - ButtonData::axisMax) * -1;

	// Mutiply by twice the radius and then subtract the radius to get the middle
	if(isLeftJoystick) {
		inputInstance->getCurrentFrame()->LS_X = scaledX;
		inputInstance->getCurrentFrame()->LS_Y = scaledY;
	} else {
		inputInstance->getCurrentFrame()->RS_X = scaledX;
		inputInstance->getCurrentFrame()->RS_Y = scaledY;
	}

	correctForCircleLock();

	event.Skip();

	Refresh();
}

void JoystickCanvas::onMouseDrag(wxMouseEvent& event) {
	if(event.Dragging()) {
		// Pass it on
		onMouseClick(event);
	}
}

void JoystickCanvas::xValueSet(wxSpinEvent& event) {
	int position = event.GetPosition();
	if(position > ButtonData::axisMax) {
		position = ButtonData::axisMax;
	}

	if(position < ButtonData::axisMin) {
		position = ButtonData::axisMin;
	}

	if(isLeftJoystick) {
		inputInstance->getCurrentFrame()->LS_X = position;
		correctForCircleLock();
	} else {
		inputInstance->getCurrentFrame()->RS_X = position;
		correctForCircleLock();
	}
	Refresh();
}

void JoystickCanvas::yValueSet(wxSpinEvent& event) {
	int position = event.GetPosition();
	if(position > ButtonData::axisMax) {
		position = ButtonData::axisMax;
	}

	if(position < ButtonData::axisMin) {
		position = ButtonData::axisMin;
	}

	if(isLeftJoystick) {
		inputInstance->getCurrentFrame()->LS_Y = position;
		correctForCircleLock();
		event.SetPosition(inputInstance->getCurrentFrame()->LS_Y);
	} else {
		inputInstance->getCurrentFrame()->RS_Y = position;
		correctForCircleLock();
		event.SetPosition(inputInstance->getCurrentFrame()->RS_Y);
	}
	Refresh();
}

void JoystickCanvas::setXValue(int32_t x) {
	if(isLeftJoystick) {
		inputInstance->getCurrentFrame()->LS_X = x;
		correctForCircleLock();
	} else {
		inputInstance->getCurrentFrame()->RS_X = x;
		correctForCircleLock();
	}
	Refresh();
}

void JoystickCanvas::setYValue(int32_t y) {
	if(isLeftJoystick) {
		inputInstance->getCurrentFrame()->LS_Y = y;
		correctForCircleLock();
	} else {
		inputInstance->getCurrentFrame()->RS_Y = y;
		correctForCircleLock();
	}
	Refresh();
}

BottomUI::BottomUI(wxFrame* parentFrame, rapidjson::Document* settings, std::shared_ptr<ButtonData> buttons, wxBoxSizer* theGrid, DataProcessing* input) {
	// TODO set up joysticks
	buttonData   = buttons;
	mainSettings = settings;

	inputInstance = input;

	parent = parentFrame;

	// Game frame viewer

	mainSizer          = new wxBoxSizer(wxVERTICAL);
	horizontalBoxSizer = new wxBoxSizer(wxHORIZONTAL);

	leftJoystickDrawer = new JoystickCanvas(parentFrame, inputInstance, true);
	leftJoystickDrawer->setBackgroundColor(*wxWHITE);
	rightJoystickDrawer = new JoystickCanvas(parentFrame, inputInstance, false);
	rightJoystickDrawer->setBackgroundColor(*wxWHITE);

	wxSize gridSize;
	// Just to get a rough estimate
	int w = buttonData->KeyWidth * buttonData->buttonMapping[Btn::A]->resizedGridOffBitmap->GetWidth();
	int h = buttonData->KeyHeight * buttonData->buttonMapping[Btn::A]->resizedGridOffBitmap->GetHeight();
	gridSize.SetWidth(w);
	gridSize.SetHeight(h);

	buttonGrid = new ButtonGrid(parentFrame, gridSize, buttonData, inputInstance);

	frameViewerCanvas = new FrameViewerCanvas(parentFrame, new wxBitmap(HELPERS::resolvePath((*mainSettings)["videoViewerDefaultImage"].GetString()), wxBITMAP_TYPE_JPEG));

	inputInstance->setInputCallback(std::bind(&BottomUI::refreshDataViews, this));

	// Add the joystick submenu
	joystickSubMenu = new wxMenu();
	joystickSubMenu->Bind(wxEVT_MENU_OPEN, &BottomUI::onJoystickMenuOpen, this);
	currentJoyDefined = false;
	lastButtonState   = 0;

	// These take up much less space than the grid
	horizontalBoxSizer->Add(leftJoystickDrawer->getSizer(), 0, wxSHAPED | wxEXPAND);
	horizontalBoxSizer->Add(rightJoystickDrawer->getSizer(), 0, wxSHAPED | wxEXPAND);

	horizontalBoxSizer->Add(buttonGrid, 0, wxSHAPED | wxEXPAND);

	// Proportion HAS to be zero here, it's a requirment
	mainSizer->Add(frameViewerCanvas, 0, wxSHAPED | wxEXPAND | wxALIGN_CENTER_HORIZONTAL);
	mainSizer->Add(horizontalBoxSizer, 1, wxEXPAND | wxALL);

	theGrid->Add(mainSizer, 3, wxEXPAND | wxALL);
}

void BottomUI::refreshDataViews() {
	// Just refresh the grid and the joysticks
	leftJoystickDrawer->Refresh();
	rightJoystickDrawer->Refresh();
	buttonGrid->Refresh();
}

void BottomUI::onJoystickSelect(wxCommandEvent& event) {
	if(currentJoyDefined) {
		// Disable the earlier one
		currentJoy->ReleaseCapture();
		delete currentJoy;
		currentJoyDefined = false;
	}
	// Joystick selected, get the index to know which joy
	currentJoy = new wxJoystick(event.GetId() - joystickSubmenuIDBase);
	// Prepare mapping
	joyButtonToSwitch.clear();
	povToSwitch.clear();
	axisToSwitch.clear();
	// Get mapping from hex string
	wxString hexWxString  = getJoyHexString(currentJoy);
	const char* hexString = hexWxString.ToUTF8().data();
	std::string joyMapping;
	// Check if exists and use default if not
	if((*mainSettings)["joystickMappings"].HasMember(hexString)) {
		joyMapping = (*mainSettings)["joystickMappings"][hexString].GetString();
	} else {
		// This default is for the pro controller connected to pc without drivers
		joyMapping = (*mainSettings)["joystickMappings"]["0x45E,0x28E"].GetString();
	}
	// Break it apart and do things
	std::vector<std::string> parts = HELPERS::splitString(joyMapping, ',');
	for(auto const& part : parts) {
		std::vector<std::string> inputParts = HELPERS::splitString(part, ':');
		char firstChar                      = inputParts[1].front();
		int index;
		if(firstChar == 'b') {
			// It adds one for some reason, ask SDL
			index = strtol(inputParts[1].erase(0, 1).c_str(), nullptr, 10) - 1;
			// This is a button
			if(buttonData->stringToButton.count(inputParts[0])) {
				// It's a normal button
				joyButtonToSwitch[index] = (int)buttonData->stringToButton[inputParts[0]];
			} else {
				// It's extended
				// Go beyond the end of Btn
				joyButtonToSwitch[index] = stringToButtonExtended[inputParts[0]] + Btn::BUTTONS_SIZE;
			}
		} else if(firstChar == 'h') {
			// This is a hat, usually pov
			// Hat comprises of two parts, but basically ignore the first part
			// Because there is usually just one hat
			// The charactr is a bit weird, translate it
			// #define 	SDL_HAT_UP   0x01
			// #define 	SDL_HAT_RIGHT   0x02
			// #define 	SDL_HAT_DOWN   0x04
			// #define 	SDL_HAT_LEFT   0x08
			char indexChar = inputParts[1].at(3);
			if(indexChar == '1') {
				index = 0;
			} else if(indexChar == '2') {
				index = 1;
			} else if(indexChar == '4') {
				index = 2;
			} else if(indexChar == '8') {
				index = 3;
			}
			// wxWidgets just does this https://docs.wxwidgets.org/3.0/classwx_joystick.html#a10712042f8cbca788ef04e96eab375a4
			if(buttonData->stringToButton.count(inputParts[0])) {
				// It's a normal button
				povToSwitch[index] = (int)buttonData->stringToButton[inputParts[0]];
			} else {
				// It's extended
				// Go beyond the end of Btn
				povToSwitch[index] = stringToButtonExtended[inputParts[0]] + Btn::BUTTONS_SIZE;
			}
		} else if(firstChar == 'a') {
			// This is an axis
			index = strtol(inputParts[1].erase(0, 1).c_str(), nullptr, 10);
			// Whether to flip the axis or not
			bool direction       = inputParts[1].at(1) == '+';
			axisDirection[index] = direction;
			// This is a button
			if(buttonData->stringToButton.count(inputParts[0])) {
				// It's a normal button
				axisToSwitch[index] = (int)buttonData->stringToButton[inputParts[0]];
			} else {
				// It's extended
				// Go beyond the end of Btn
				axisToSwitch[index] = stringToButtonExtended[inputParts[0]] + Btn::BUTTONS_SIZE;
			}
		}
	}
	// Finally, start listening
	currentJoy->SetCapture(parent);
	currentJoyDefined = true;
}

void BottomUI::onJoystickMenuOpen(wxMenuEvent& event) {
	// Remove all current items
	int currentNumOfItems = joystickSubMenu->GetMenuItemCount();
	for(int i = 0; i < currentNumOfItems; i++) {
		joystickSubMenu->Delete(i + joystickSubmenuIDBase);
	}
	// Get number of currently selected joysticks and list them
	int numOfConnectedJoysticks = wxJoystick::GetNumberJoysticks();
	for(int i = 0; i < numOfConnectedJoysticks; i++) {
		// Briefly open it for info
		// Not using wxJOYSTICK1 or wxJOYSTICK2, because that restricts me to two
		wxJoystick* joy = new wxJoystick(i);
		// wxString name   = joy->GetProductName();
		wxString printString = getJoyHexString(joy);
		joystickSubMenu->Append(i + joystickSubmenuIDBase, printString);
		// Delete it because it doesn't need to be used right now
		delete joy;
	}
}

wxString BottomUI::getJoyHexString(wxJoystick* joy) {
	int manufacturerID = joy->GetManufacturerId();
	int productID      = joy->GetProductId();
	// Convert each to a hex string using sprintf, to save on bloat from stringstream
	char buffer[10];
	sprintf(buffer, "0x%X,0x%X", manufacturerID, productID);
	return wxString(buffer);
}

void BottomUI::onJoystickChange(wxJoystickEvent& event) {
	// Handle joystick events live
	// Will be really complicated, so use this https://robsears.com/ultimate-wxjoystick-tutorial/
	// Use https://github.com/gabomdq/gamecontrollerdb for mapping
	// Check this too https://gaming.stackexchange.com/a/358794
	if(event.IsButton() && event.ButtonIsDown()) {
		// Button down event
		// Check POV too just in case
		int buttonState = currentJoy->GetButtonState();
		for(int i = 0; i < currentJoy->GetNumberButtons(); i++) {
			// Get the value with bitwise
			uint8_t isPressed = GET_BIT(buttonState, i);
			if(isPressed && !GET_BIT(lastButtonState, i)) {
				// This button wasn't clicked before and now it is, trigger it
				// Essentially, it's a button down event for this button
				if(joyButtonToSwitch.count(i)) {
					Btn button = (Btn)joyButtonToSwitch[i];
					inputInstance->handleButtonInput(button);
				}
			}
		}
	} else if(event.IsMove() || event.IsZMove()) {
		// Move event, I think any axis
		// Check each axis individually
		for(int i = 0; i < currentJoy->GetNumberAxes(); i++) {
			// Range should be from -32768 to +32768 or 0 to +65535
			// Ranges should be the same for all axis
			int axisMin    = currentJoy->GetXMin();
			int axisMax    = currentJoy->GetXMax();
			int axisMiddle = std::floor((axisMin + axisMax) / 2.0f);
			int axisValue  = currentJoy->GetPosition(i);

			// Normalize to 1, then multiply by the range
			int32_t normalizedAxisValue = ((float)(axisValue - axisMiddle) / (float)(axisMax - axisMiddle)) * ButtonData::axisMax;

			// Flip if needed
			if(!axisDirection[i]) {
				normalizedAxisValue *= -1;
			}

			if(axisToSwitch.count(i)) {
				int switchID = axisToSwitch[i];
				if(switchID < Btn::BUTTONS_SIZE) {
					// Check RS and LS only because it's susceptible to this
					if((Btn)switchID == Btn::RS || (Btn)switchID == Btn::LS) {
						if(axisLastState[i] < 0 && axisValue > axisMiddle) {
							// Trigger RS
							inputInstance->handleButtonInput(Btn::RS);
							axisLastState[i] = 1;
						} else if(axisLastState[i] > 0 && axisValue < axisMiddle) {
							// Trigger LS
							inputInstance->handleButtonInput(Btn::LS);
							axisLastState[i] = -1;
						}
					}
				} else {
					// This is an extended value, a joystick value
					// Just set it as a normal axis
					int axisID = switchID - Btn::BUTTONS_SIZE;
					// AXIS VALUE ABSOLUTELY WRONG TODO
					if(axisID == 0) {
						// LSX
						leftJoystickDrawer->setXValue(normalizedAxisValue);
					} else if(axisID == 1) {
						// LSY
						leftJoystickDrawer->setYValue(normalizedAxisValue);
					} else if(axisID == 2) {
						// RSX
						rightJoystickDrawer->setXValue(normalizedAxisValue);
					} else if(axisID == 3) {
						// RSY
						rightJoystickDrawer->setYValue(normalizedAxisValue);
					}
				}
			}
		}
	}

	// I don't know what event it falls under
	// https://docs.wxwidgets.org/3.0/classwx_joystick.html#a10712042f8cbca788ef04e96eab375a4
	int povValue = currentJoy->GetPOVPosition() / 9000;
	int switchID = povToSwitch[povValue];
	if(povLastState != povValue) {
		if(switchID < Btn::BUTTONS_SIZE) {
			// Normal button
			// Won't check for axis, too scared
			Btn button = (Btn)switchID;
			inputInstance->handleButtonInput(button);
		}
		povLastState = povValue;
	}
}