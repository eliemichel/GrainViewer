// **************************************************
// Author : Élie Michel <elie.michel@telecom-paristech.fr>
// UNPUBLISHED CODE.
// Copyright (C) 2017 Élie Michel.
// **************************************************

#pragma once

/**
 * When deriving from this, you may call
 * registerDialogForBehavior(DialogType, BehaviorType) for the dialog to be
 * automatically created when a behavior is associated to an object of the
 * scene.
 * In this case, the Dialog must define a function
 *     void setControlledBehavior(std::weak_ptr<BehaviorType>)
 */
class Dialog {
public:
	virtual void draw() {}
};

template <typename T> struct DialogFactory { static std::shared_ptr<Dialog> MakeShared() { return std::make_shared<Dialog>(); } };
#define registerDialogForBehavior(DialogType, BehaviorType) template<> struct DialogFactory<BehaviorType> { static std::shared_ptr<DialogType> MakeShared() { return std::make_shared<DialogType>(); } };

