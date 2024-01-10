/*  ===========================================================================
 *
 *   This file is part of HISE.
 *   Copyright 2016 Christoph Hart
 *
 *   HISE is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   HISE is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Commercial licenses for using HISE in an closed source project are
 *   available on request. Please visit the project's website to get more
 *   information about commercial licensing:
 *
 *   http://www.hise.audio/
 *
 *   HISE is based on the JUCE library,
 *   which must be separately licensed for closed source applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

#include <JuceHeader.h>
#include "MultiPageDialog.h"

namespace hise
{
using namespace juce;

namespace PageFactory
{

#define DEFAULT_PROPERTIES(x) SN_NODE_ID(#x); DefaultProperties getDefaultProperties() const override { return getStaticDefaultProperties(); } \
    static DefaultProperties getStaticDefaultProperties()

struct MarkdownText: public MultiPageDialog::PageBase
{
    DEFAULT_PROPERTIES(MarkdownText)
    {
        return { { MultiPageIds::Text, "### funkyNode" } };
    }
    
    MarkdownText(MultiPageDialog& r, int width, const var& d);

    void createEditorInfo(MultiPageDialog::PageInfo* rootList) override;
    
    void postInit() override;
    void paint(Graphics& g) override;
    Result checkGlobalState(var) override;

private:

    var obj;
    MarkdownRenderer r;
};

struct FileSelector: public MultiPageDialog::PageBase
{
    DEFAULT_PROPERTIES(FileSelector)
    {
        return {
            { MultiPageIds::Directory, true },
            { MultiPageIds::ID, "fileId" },
            { MultiPageIds::Wildcard, "*.*" },
            { MultiPageIds::SaveFile, true }
        };
    }
    
    void createEditorInfo(MultiPageDialog::PageInfo* rootList) override
    {
        rootList->addChild<MarkdownText>({
            { { MultiPageIds::Text, "oink? "} }
        });
    }
    
    FileSelector(MultiPageDialog& r, int width, const var& obj);
    
    void postInit() override;
    Result checkGlobalState(var globalState) override;

    static FilenameComponent* createFileComponent(const var& obj);
    static File getInitialFile(const var& path);

    void resized() override;

private:
    
    bool isDirectory = false;
    ScopedPointer<juce::FilenameComponent> fileSelector;
    Identifier fileId;
};

struct LabelledComponent: public MultiPageDialog::PageBase
{
    LabelledComponent(MultiPageDialog& r, int width, const var& obj, Component* c):
      PageBase(r, width, obj),
      component(c)
    {
        addAndMakeVisible(c);
        label = obj[MultiPageIds::Text].toString();
        
        setSize(width, 32);
    };
    
    void paint(Graphics& g) override
    {        
        auto b = getLocalBounds();
        auto df = MultiPageDialog::getDefaultFont(*this);
            
        g.setFont(df.first);
        g.setColour(df.second);
            
        g.drawText(label, b.toFloat().reduced(8.0f, 0.0f), Justification::left);
    }
    
    template <typename T> T& getComponent() { return *dynamic_cast<T*>(component.get()); }
    
    void resized() override
    {
        auto b = getLocalBounds();
        
        if(helpButton != nullptr)
        {
            helpButton->setBounds(b.removeFromRight(32).withSizeKeepingCentre(24, 24));
            b.removeFromRight(5);
        }

        b.removeFromLeft(getWidth() / 4);
        
        component->setBounds(b);
    }
    
    String label;
    ScopedPointer<Component> component;
};

struct TextInput: public LabelledComponent,
                  public TextEditor::Listener,
                  public Timer
{
    DEFAULT_PROPERTIES(Choice)
    {
        return {
            { MultiPageIds::Text, "Label" },
            { MultiPageIds::ID, "textId" },
            { MultiPageIds::Help, "" },
            { MultiPageIds::Required, false },
            { MultiPageIds::Items, var(Array<var>({var("Autocomplete 1"), var("Autocomplete 2")})) }
        };
    }

    struct Autocomplete: public Component,
                         public ScrollBar::Listener,
                         public ComponentMovementWatcher
    {
        struct Item
        {
            String displayString;
        };
        
        ScrollBar sb;
        ScrollbarFader fader;
           
        static constexpr int ItemHeight = 28;
        
        void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& details)
        {
            sb.mouseWheelMove(e, details);
        }
        
        void scrollBarMoved (ScrollBar* scrollBarThatHasMoved,
                                     double newRangeStart) override
        {
            repaint();
        }
        
        Font f;
        
        void componentMovedOrResized (bool wasMoved, bool wasResized) override
        {
            dismiss();
        }

        /** This callback happens when the component's top-level peer is changed. */
        void componentPeerChanged() {};

        /** This callback happens when the component's visibility state changes, possibly due to
            one of its parents being made visible or invisible.
        */
        void componentVisibilityChanged() override
        {
            if(getComponent()->isShowing())
                dismiss();
        }
        
        Autocomplete(TextInput& p):
          ComponentMovementWatcher(&p),
          parent(&p),
          sb(true)
        {
            f = MultiPageDialog::getDefaultFont(*parent).first;
            
            sb.addListener(this);
            addAndMakeVisible(sb);
            fader.addScrollBarToAnimate(sb);
            
            sb.setSingleStepSize(0.2);
            
            auto& ed = parent->getComponent<TextEditor>();
            
            update(ed.getText());
            
            setSize(ed.getWidth() + 20, ItemHeight * 4 + 5 + 20);
            
            setWantsKeyboardFocus(true);
            parent->getTopLevelComponent()->addChildComponent(this);
            auto topLeft = ed.getTopLevelComponent()->getLocalArea(&ed, ed.getLocalBounds()).getTopLeft();
            
            setTopLeftPosition(topLeft.getX() - 10, topLeft.getY() + ed.getHeight());

            Desktop::getInstance().getAnimator().fadeIn(this, 150);
            
        }
        
        ~Autocomplete()
        {
            setComponentEffect(nullptr);
        }
        
        int selectedIndex = 0;
        
        void mouseDown(const MouseEvent& e) override
        {
            auto newIndex = sb.getCurrentRangeStart() + (e.getPosition().getY() - 15) / ItemHeight;
            
            if(isPositiveAndBelow(newIndex, items.size()))
                setSelectedIndex(newIndex);
        }
        
        void mouseDoubleClick(const MouseEvent& e) override
        {
            setAndDismiss();
        }
        
        bool inc(bool next)
        {
            auto newIndex = selectedIndex + (next ? 1 : -1);
            
            if(isPositiveAndBelow(newIndex, items.size()))
            {
                setSelectedIndex(newIndex);
                return true;
            }
            
            return false;
        }
        
        bool keyPressed(const KeyPress& k)
        {
            if(k == KeyPress::upKey)
                return inc(false);
            if(k == KeyPress::downKey)
                return inc(true);
            if(k == KeyPress::escapeKey)
                return dismiss();
            if(k == KeyPress::returnKey ||
               k == KeyPress::tabKey)
                return setAndDismiss();
            
            return false;
        }
        
        void setSelectedIndex(int index)
        {

            
            selectedIndex = index;
            
            if(!sb.getCurrentRange().contains(selectedIndex))
            {
                if(sb.getCurrentRange().getStart() > selectedIndex)
                    sb.setCurrentRangeStart(selectedIndex);
                else
                    sb.setCurrentRangeStart(selectedIndex - 3);
            }
            
            repaint();
        }
        
        void resized() override
        {
            sb.setBounds(getLocalBounds().reduced(10).removeFromRight(16).reduced(1));
        }
        
        void paint(Graphics& g) override
        {
            auto b = getLocalBounds().toFloat();
             
            b = b.reduced(10.0f);
            
            DropShadow sh;
            sh.colour = Colours::black.withAlpha(0.7f);
            sh.radius = 10;
            sh.drawForRectangle(g, b.toNearestInt());
                
            b.reduced(2.0f);
            g.setColour(Colour(0xFF222222));
            g.fillRoundedRectangle(b, 5.0f);
            g.setColour(Colours::white.withAlpha(0.3f));
            g.drawRoundedRectangle(b, 5.0f, 2.0f);
            
            auto r = sb.getCurrentRange();
            
            b.reduced(5.0f);
            b.removeFromLeft(10.0f);
            b.removeFromRight(sb.getWidth());

            b.removeFromTop(2.5f);
            
            g.setFont(f);
            
            if(items.isEmpty())
            {
                g.setColour(Colours::white.withAlpha(0.1f));
                g.drawText("No items found", getLocalBounds().toFloat(), Justification::centred);
            }
            else
            {
                for(int i = 0; i < 4; i++)
                {
                    g.setColour(Colours::white.withAlpha(0.6f));
                    auto tb = b.removeFromTop(ItemHeight);
                    
                    auto idx = i + roundToInt(r.getStart());
                    
                    if(idx == selectedIndex)
                    {
                        g.fillRoundedRectangle(tb.withX(10.0f).reduced(3.0f, 1.0f), 3.0f);
                        g.setColour(Colours::black.withAlpha(0.8f));
                    }
                        
                    g.drawText(items[idx].displayString, tb, Justification::left);
                }
            }
        }
        
        bool setAndDismiss()
        {
            auto newTextAfterComma = items[selectedIndex].displayString;
            auto& ed = parent->getComponent<TextEditor>();
            
            String nt = ed.getText();
                
            if(nt.containsChar(','))
            {
                nt = nt.upToLastOccurrenceOf(",", false, false);
                nt << ", " << newTextAfterComma;
            }
            else
                nt = newTextAfterComma;
            
            ed.setText(nt, sendNotificationSync);
            
            return dismiss();
        }
        
        bool dismiss()
        {
            SafeAsyncCall::call<TextInput>(*parent, [](TextInput& ti)
            {
                ti.dismissAutocomplete();
                ti.getComponent<TextEditor>().grabKeyboardFocusAsync();
            });
            
            return true;
        }
        
        void update(const String& currentText)
        {
            auto search = currentText.fromLastOccurrenceOf(",", false, false).toLowerCase().trim();
            
            items.clear();
            
            for(const auto& i: allItems)
            {
                if(search.isEmpty() || i.displayString.toLowerCase().contains(search))
                {
                    items.add(i);
                }
            }
            
            sb.setRangeLimits(0.0, (double)items.size());
            sb.setCurrentRange(0.0, 4.0);
            setSelectedIndex(0);
            
            if(items.isEmpty())
                dismiss();
        }
        
        Array<Item> allItems;
        Array<Item> items;
        
        WeakReference<TextInput> parent;
    };
    
    ScopedPointer<Autocomplete> currentAutocomplete;
    
    void showAutocomplete(const String& currentText)
    {
        if(currentAutocomplete == nullptr && currentText.isNotEmpty())
        {
            currentAutocomplete = new Autocomplete(*this);
        }
        else
        {
            if(currentText.isEmpty())
                currentAutocomplete = nullptr;
            else
                currentAutocomplete->update(currentText);
        }
    }
    
    void textEditorEscapeKeyPressed(TextEditor& e)
    {
        if(currentAutocomplete != nullptr)
            dismissAutocomplete();
        else
            currentAutocomplete = new Autocomplete(*this);
    }
    
    void dismissAutocomplete()
    {
        stopTimer();
        Desktop::getInstance().getAnimator().fadeOut(currentAutocomplete, 150);
        currentAutocomplete = nullptr;
    }
    
    void textEditorFocusLost(TextEditor& e)
    {
        //currentAutocomplete = nullptr;
    }
    
    bool keyPressed(const KeyPress& k) override
    {
        if(currentAutocomplete == nullptr)
            return false;
        
        if(k == KeyPress::upKey)
            return currentAutocomplete->inc(false);
        if(k == KeyPress::downKey)
            return currentAutocomplete->inc(true);
            
        return false;
    }
    
    void timerCallback() override
    {
        if(Component::getCurrentlyFocusedComponent() == &getComponent<TextEditor>())
            showAutocomplete(getComponent<TextEditor>().getText());
        
        stopTimer();
    }
    
    void textEditorReturnKeyPressed(TextEditor& e)
    {
        if(currentAutocomplete != nullptr)
            currentAutocomplete->setAndDismiss();
    }
    
    void textEditorTextChanged(TextEditor& e)
    {
        //check(MultiPageDialog::getGlobalState(*this, id, var()));
        
        writeState(e.getText());
        
        startTimer(400);
    }
    
    TextInput(MultiPageDialog& r, int width, const var& obj);;

    void postInit() override;
    
    Result checkGlobalState(var globalState) override;

private:
    
    bool required = false;
    
    JUCE_DECLARE_WEAK_REFERENCEABLE(TextInput);
};

struct Tickbox: public LabelledComponent,
                public ButtonListener
{
    DEFAULT_PROPERTIES(Tickbox)
    {
        return {
            { MultiPageIds::Text, "Label" },
            { MultiPageIds::ID, "tickId" },
            { MultiPageIds::Help, "" },
            { MultiPageIds::Required, false }
        };
    }
    
    void createEditorInfo(MultiPageDialog::PageInfo* rootList) override
    {
        rootList->addChild<MarkdownText>({
            { MultiPageIds::Text, "Tickbox" }
        });
        
        rootList->addChild<TextInput>({
            { MultiPageIds::ID, "ID" },
            { MultiPageIds::Text, "ID" },
            { MultiPageIds::Help, "The ID for the element (used as key in the state `var`.\n> If you have multiple tickboxes on the same page with the same ID, they will be put into a radio group and can be selected exclusively (also the ID value will be the integer index of the button in the group list in the order of appearance." }
        });
        
        rootList->addChild<TextInput>({
            { MultiPageIds::ID, "Text" },
            { MultiPageIds::Text, "Text" },
            { MultiPageIds::Help, "The label next to the tickbox" }
        });
        
        rootList->addChild<Tickbox>({
            { MultiPageIds::ID, "Required" },
            { MultiPageIds::Text, "Required" },
            { MultiPageIds::Help, "If this is enabled, the tickbox must be selected in order to proceed to the next page, otherwise it will show a error message.\n> This is particularly useful for eg. accepting TOC" }
        });
        
    }

    Tickbox(MultiPageDialog& r, int width, const var& obj);;

    void postInit() override;

    Result checkGlobalState(var globalState) override;

    void buttonClicked(Button* b) override;
    
private:
    
    Array<ToggleButton*> groupedButtons;
    int thisRadioIndex = -1;
    
    bool required = false;
    bool requiredOption = false;
};

struct Choice: public LabelledComponent
{
    DEFAULT_PROPERTIES(Choice)
    {
        return {
            { MultiPageIds::Text, "Label" },
            { MultiPageIds::ID, "choiceId" },
            { MultiPageIds::Help, "" },
            { MultiPageIds::Items, var(Array<var>({var("Option 1"), var("Option 2")})) }
        };
    }


    
    Choice(MultiPageDialog& r, int width, const var& obj):
      LabelledComponent(r, width, obj, new ComboBox())
    {
        auto& combobox = getComponent<ComboBox>();
        auto s = obj[MultiPageIds::Items].toString();
        combobox.addItemList(StringArray::fromLines(s), 1);
        hise::GlobalHiseLookAndFeel::setDefaultColours(combobox);
    };

    void postInit() override
    {
        auto t = getValueFromGlobalState().toString();
        getComponent<ComboBox>().setText(t, dontSendNotification);
    }
    
    Result checkGlobalState(var globalState) override
    {
        return Result::ok();
    }
private:
    
};




struct Container: public MultiPageDialog::PageBase
{
    Container(MultiPageDialog& r, int width, const var& obj);;
    virtual ~Container() {};

    void postInit() override;
    Result checkGlobalState(var globalState) override;

    
    
    virtual void calculateSize() = 0;

    void addChild(MultiPageDialog::PageInfo::Ptr info);

protected:

    OwnedArray<PageBase> childItems;
    MultiPageDialog::PageInfo::List staticPages;

private:

    void addChild(int width, const var& r);
    MultiPageDialog::Factory factory;
};

struct List: public Container
{
    DEFAULT_PROPERTIES(List)
    {
        return {
            { MultiPageIds::ID, "listId" },
            { MultiPageIds::Text, "Title" },
            { MultiPageIds::Padding, true },
            { MultiPageIds::Foldable, false },
            { MultiPageIds::Folded, false }
        };
    }

    void calculateSize() override;

    List(MultiPageDialog& r, int width, const var& obj);
    void resized() override;
    
    void createEditorInfo(MultiPageDialog::PageInfo* info) override;
    
    
    void mouseDown(const MouseEvent& e)
    {
        if(foldable && e.getPosition().getY() < titleHeight)
        {
            folded = !folded;
            
            Container* c = this;
            
            c->calculateSize();
            repaint();
            
            while(c = dynamic_cast<Container*>(c->getParentComponent()))
            {
                c->calculateSize();
            }
        }
    }
    
    
    
    void paint(Graphics& g) override
    {
        if(foldable)
        {
            if(auto laf = dynamic_cast<MultiPageDialog::LookAndFeelMethods*>(&rootDialog.getLookAndFeel()))
            {
                auto b = getLocalBounds().removeFromTop(titleHeight).toFloat();
                laf->drawMultiPageFoldHeader(g, *this, b, title, folded);
            }
        }
    }
    
    int titleHeight = 32;
    Path fold;
    String title;
    bool foldable = false;
    bool folded = false;
};

struct Action: public MultiPageDialog::PageBase
{
    enum class CallType
    {
        Asynchronous,
        Synchronous,
        BackgroundThread
    };
    
    SN_NODE_ID("Action");
    
    Action(MultiPageDialog& r, int, const var& obj):
      PageBase(r, 0, obj),
      r(Result::ok())
    {
        auto ct = obj[MultiPageIds::CallType];
            
        if(ct.isString())
        {
            auto idx = jmax(0, StringArray({"Async", "Sync", "BackgroundThread"}).indexOf(ct.toString()));
            
            callType = (CallType)idx;
        }
        else
            callType = (CallType)(int)ct;
    }
    
    void postInit() override
    {
        switch(callType)
        {
            case CallType::Synchronous: perform(); break;
            case CallType::Asynchronous: MessageManager::callAsync(BIND_MEMBER_FUNCTION_0(Action::perform)); break;
            //case CallType::BackgroundThread: rootDialog.getRunThread().addJob(this); break;
        }
    }
    
    void perform()
    {
        auto obj = MultiPageDialog::getGlobalState(*this, {}, var());
        
        CustomCheckFunction f;
        std::swap(f, cf);
        r = f(this, obj);
    }
    
    Result checkGlobalState(var globalState) override
    {
        return r;
    }
    
    Result r;
    CallType callType = CallType::Asynchronous;
};

struct Skip: public Action
{
    Skip(MultiPageDialog& r, int w, const var& obj):
      Action(r, w, obj)
    {
        auto direction = r.getCurrentNavigationDirection();
        
        setCustomCheckFunction([direction](MultiPageDialog::PageBase* pb, const var& obj)
        {
            auto rd = &pb->rootDialog;
            MessageManager::callAsync([rd, direction]()
            {
                rd->navigate(direction);
            });
                       
            return Result::ok();
        });
    };
};

struct DummyWait: public Action
{
    SN_NODE_ID("DummyWait");
    
    using Job = MultiPageDialog::RunThread::Job;
    
    struct WaitJob: public MultiPageDialog::RunThread::Job
    {
        WaitJob(MultiPageDialog::RunThread& r, const var& obj):
          Job(r, obj)
        {};
        
        Result run() override
        {
            for(int i = 0; i < 100; i++)
            {
                if(parent.threadShouldExit())
                    return Result::fail("aborted");
                
                progress = (double)i / 99.0;
                parent.wait(30);
                

                if(i == 80 && false)
                    return abort("**Lost connection**.  \nPlease ensure that your internet connection is stable and click the retry button to resume the download process.");
            }
            
            return Result::ok();

        }
        
        Result abort(const String& message)
        {
            if(currentPage != nullptr)
            {
                SafeAsyncCall::call<DummyWait>(*currentPage, [](DummyWait& w)
                {
                    w.rootDialog.setCurrentErrorPage(&w);
                    w.retryButton.setVisible(true);
                    w.resized();
                });
            }
            
            return Result::fail(message);
        }
        
        WeakReference<DummyWait> currentPage;
    };
    
    Job::Ptr job;
    
    DummyWait(MultiPageDialog& r, int w, const var& obj):
      Action(r, w, obj),
      retryButton("retry", nullptr, r)
    {
        addChildComponent(retryButton);
        callType = CallType::BackgroundThread;
        
        job = r.getJob(obj);
        
        if(job == nullptr)
        {
            job = new WaitJob(r.getRunThread(), obj);
        }
        
        dynamic_cast<WaitJob*>(job.get())->currentPage = this;
        
        addAndMakeVisible(progress = new ProgressBar(job->getProgress()));
        
        progress->setOpaque(false);
        
        retryButton.onClick = [this]()
        {
            rootDialog.getRunThread().addJob(job, true);
            retryButton.setVisible(false);
            resized();
        };
        
        label = obj[MultiPageIds::Text].toString();
        
        addAndMakeVisible(progress);
        setSize(w, 32);
    }
    
    
    
    void paint(Graphics& g) override
    {
        if(label.isNotEmpty())
        {
            auto b = getLocalBounds();
            auto df = MultiPageDialog::getDefaultFont(*this);
                
            g.setFont(df.first);
            g.setColour(df.second);
                
            g.drawText(label, b.toFloat().reduced(8.0f, 0.0f), Justification::left);
        }
    }
    
    void resized() override
    {
        auto b = getLocalBounds().reduced(3, 0);
        
        if(label.isNotEmpty())
            b.removeFromLeft(b.getWidth() / 4);
        
        if(retryButton.isVisible())
            retryButton.setBounds(b.removeFromRight(b.getHeight()).withSizeKeepingCentre(24, 24));
        
        progress->setBounds(b);
    }
    
    String label;
    ScopedPointer<ProgressBar> progress;
    HiseShapeButton retryButton;
    
    JUCE_DECLARE_WEAK_REFERENCEABLE(DummyWait);
};

struct Column: public Container
{
    DEFAULT_PROPERTIES(Column)
    {
        return {
            { MultiPageIds::ID, "columnId" },
            { MultiPageIds::Padding, true },
            { MultiPageIds::Width, var(Array<var>({var(-0.5), var(-0.5)})) }
        };
    }

    Column(MultiPageDialog& r, int width, const var& obj);

    void calculateSize() override;
	void resized() override;

    void postInit() override
    {
        if(!staticPages.isEmpty())
        {
            auto equidistance = -1.0 / staticPages.size();
            
            for(auto sp: staticPages)
            {
                auto v = (*sp)[MultiPageIds::Width];
                
                if(v.isUndefined() || v.isVoid())
                    widthInfo.add(equidistance);
                else
                    widthInfo.add((double)v);
            }
        }
	    
        Container::postInit();
    }

	Array<double> widthInfo;
};

struct Branch: public Container
{
    
    DEFAULT_PROPERTIES(Branch)
    {
        return {
            { MultiPageIds::ID, "branchId" }
        };
    }
    
    Branch(MultiPageDialog& root, int w, const var& obj):
      Container(root, w, obj)
    {
        setSize(w, 0);
    };
    
    PageBase* createWithPopup(int width)
    {
        PopupLookAndFeel plaf;
        PopupMenu m;
        m.setLookAndFeel(&plaf);
        
        int index = 0;
        
        for(auto sp: staticPages)
        {
            m.addItem(index + 1, sp->getData()[MultiPageIds::Text].toString());
            index++;
        }
        
        auto result = m.show();
        
        if(isPositiveAndBelow(result-1, staticPages.size()))
        {
            if(auto sp = staticPages[result - 1])
                return sp->create(rootDialog, width);
        }
        
        return nullptr;
    }
    
    void postInit() override
    {
        currentIndex = getValueFromGlobalState();
        
        for(const auto& sp: staticPages)
        {
            childItems.add(sp->create(rootDialog, getWidth()));
            addAndMakeVisible(childItems.getLast());
        }
            
        if(PageBase* p = childItems.removeAndReturn(currentIndex))
        {
            childItems.clear();
            childItems.add(p);
            p->postInit();
        }
        else
            childItems.clear();
        
        calculateSize();
    }
    
    void calculateSize() override
    {
        if(auto c = childItems[0])
        {
            c->setVisible(true);
            setSize(getWidth(), c->getHeight());
        }
    }
    
    Result checkGlobalState(var globalState) override
    {
        if(auto p = childItems[0])
            return p->check(globalState);
        
        return Result::fail("No branch selected");
    }
    
    void resized() override
    {
        if(auto p = childItems[0])
            p->setBounds(getLocalBounds());
    }
    
    int currentIndex = 0;
    
};

struct Builder: public Container,
                public ButtonListener
{
    DEFAULT_PROPERTIES(Builder)
    {
        return {
            { MultiPageIds::ID, "columnId" },
            { MultiPageIds::Text, "title" },
            { MultiPageIds::Folded, false },
            { MultiPageIds::Padding, 10 }
        };
    }
    
    Builder(MultiPageDialog& r, int w, const var& obj):
      Container(r, w, obj),
      addButton("add", nullptr, r)
    {
        title = obj[MultiPageIds::Text];
        padding = jmax(padding, 10);
        
        childItems.clear();
        addAndMakeVisible(addButton);
        addButton.onClick = BIND_MEMBER_FUNCTION_0(Builder::onAddButton);
        setSize(w, 32);
    }
    
    var stateList;
    
    void mouseDown(const MouseEvent& e) override
    {
        if(e.getPosition().getY() < 32)
        {
            folded = !folded;
            rebuildPosition();
            repaint();
        }
    }
    
    bool folded = false;
    
    void calculateSize() override
    {
        int h = 32 + padding;
        
        for(auto& c: childItems)
        {
            c->setVisible(!folded);
            
            if(!folded)
                h += c->getHeight() + 3 * padding;
        }
        
        setSize(getWidth(), h);
    }
    
    void resized() override
    {
        auto b = getLocalBounds();
        
        addButton.setBounds(b.removeFromTop(32).removeFromRight(32).withSizeKeepingCentre(24, 24));
        
        b.removeFromTop(padding);
        
        itemBoxes.clear();
        
        for(int i = 0; i < childItems.size(); i++)
        {
            auto c = childItems[i];
            auto row = b.removeFromTop(2 * padding + c->getHeight());

            itemBoxes.addWithoutMerging(row);

            auto bb = row.removeFromRight(32).removeFromTop(32).withSizeKeepingCentre(20, 20);
            
            row = row.reduced(padding);
            
            closeButtons[i]->setVisible(c->isVisible());
            
            if(!folded)
            {   
                closeButtons[i]->setBounds(bb);
                c->setBounds(row);
                b.removeFromTop(padding);
            }
        }
    }
    
    RectangleList<int> itemBoxes;
    
    void paint(Graphics& g) override
    {
        for(auto& b: itemBoxes)
        {
            g.setColour(Colours::white.withAlpha(0.2f));
            g.drawRoundedRectangle(b.toFloat().reduced(1.0f), 5.0f, 2.0f);
        }
        
        if(auto laf = dynamic_cast<MultiPageDialog::LookAndFeelMethods*>(&rootDialog.getLookAndFeel()))
        {
            auto b = getLocalBounds().removeFromTop(32).toFloat();
            laf->drawMultiPageFoldHeader(g, *this, b, title, folded);
        }
    }
    
    Result checkGlobalState(var globalState) override
    {
        int idx = 0;
        
        for(auto& c: childItems)
        {
            var obj;
            
            if(isPositiveAndBelow(idx, stateList.size()))
                obj = stateList[idx];
            
            if(obj.getDynamicObject() == nullptr)
            {
                stateList.insert(idx, var(new DynamicObject()));
                obj = stateList[idx];
            }
            
            auto ok = c->check(obj);
            
            if(ok.failed())
                return ok;
            
            idx++;
        }
        
        DBG(JSON::toString(stateList));
        
        return Result::ok();
    }
    
    void addChildItem(PageBase* b, const var& stateInArray)
    {
        childItems.add(b);
        closeButtons.add(new HiseShapeButton("close", nullptr, rootDialog));
        addAndMakeVisible(closeButtons.getLast());
        addAndMakeVisible(childItems.getLast());
        closeButtons.getLast()->addListener(this);
        childItems.getLast()->setStateObject(stateInArray);
        childItems.getLast()->postInit();
    }
    
    void createItem(const var& stateInArray)
    {
        for(const auto& sp: staticPages)
        {
            auto b = sp->create(rootDialog, getWidth());
            addChildItem(b, stateInArray);
        }
    }
    
    void onAddButton()
    {
        var no(new DynamicObject());
        
        stateList.insert(-1, no);
        
        folded = false;
        
        if(popupOptions != nullptr)
        {
            if(auto nb = popupOptions->createWithPopup(getWidth()))
            {
                addChildItem(nb, no);
            }
        }
        else
        {
            createItem(no);
        }
        
        rebuildPosition();
    }
    
    void rebuildPosition()
    {
        auto t = dynamic_cast<Container*>(this);
        
        while(t != nullptr)
        {
            t->calculateSize();
            
            t = t->findParentComponentOfClass<Container>();
        }
        repaint();
    }
    
    void buttonClicked(Button* b) override
    {
        auto cb = dynamic_cast<HiseShapeButton*>(b);
        auto indexToDelete = closeButtons.indexOf(cb);
        
        stateList.getArray()->remove(indexToDelete);
        closeButtons.removeObject(cb);
        childItems.remove(indexToDelete);
        
        rebuildPosition();
    }
    
    void postInit() override
    {
        stateList = getValueFromGlobalState();
        
        if(auto firstItem = staticPages.getFirst())
        {
            ScopedPointer<PageBase> fi = firstItem->create(rootDialog, getWidth());
            
            if(auto br = dynamic_cast<Branch*>(fi.get()))
            {
                popupOptions = br;
                fi.release();
            }
        }
        
        if(!stateList.isArray())
        {
            stateList = var(Array<var>());
            writeState(stateList);
        }
        else
        {
            for(auto& obj: *stateList.getArray())
            {
                createItem(obj);
            }
        }

        calculateSize();
    }
    
    ScopedPointer<Branch> popupOptions;
    
    String title;
    HiseShapeButton addButton;
    
    OwnedArray<HiseShapeButton> closeButtons;
    
};




}

}