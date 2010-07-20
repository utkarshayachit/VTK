/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkView.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkView - The superclass for all views.
//
// .SECTION Description
// vtkView is the superclass for views.  A view is generally an area of an
// application's canvas devoted to displaying one or more VTK data objects.
// Associated representations (subclasses of vtkDataRepresentation) are
// responsible for converting the data into a displayable format.  These
// representations are then added to the view.
//
// For views which display only one data object at a time you may set a
// data object or pipeline connection directly on the view itself (e.g.
// vtkGraphLayoutView, vtkLandscapeView, vtkTreeMapView).
// The view will internally create a vtkDataRepresentation for the data.
//
// A view has the concept of linked selection.  If the same data is displayed
// in multiple views, their selections may be linked by setting the same
// vtkAnnotationLink on their representations (see vtkDataRepresentation).

#ifndef __vtkView_h
#define __vtkView_h

#include "vtkObject.h"

class vtkAlgorithmOutput;
class vtkCommand;
class vtkDataObject;
class vtkDataRepresentation;
class vtkInformation;
class vtkInformationRequestKey;
class vtkInformationVector;
class vtkSelection;
class vtkViewTheme;

class VTK_VIEWS_EXPORT vtkView : public vtkObject
{
public:
  static vtkView *New();
  vtkTypeMacro(vtkView, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Adds the representation to the view.
  void AddRepresentation(vtkDataRepresentation* rep);
  
  // Description:
  // Set the representation to the view.
  void SetRepresentation(vtkDataRepresentation* rep);
 
  // Description:
  // Convenience method which creates a simple representation with the 
  // connection and adds it to the view.
  // Returns the representation internally created.
  // NOTE: The returned representation pointer is not reference-counted, 
  // so you MUST call Register() on the representation if you want to 
  // keep a reference to it.
  vtkDataRepresentation* AddRepresentationFromInputConnection(vtkAlgorithmOutput* conn);

  // Description:
  // Convenience method which sets the representation with the 
  // connection and adds it to the view.
  // Returns the representation internally created.
  // NOTE: The returned representation pointer is not reference-counted, 
  // so you MUST call Register() on the representation if you want to 
  // keep a reference to it.
  vtkDataRepresentation* SetRepresentationFromInputConnection(vtkAlgorithmOutput* conn);

  // Description:
  // Convenience method which creates a simple representation with the 
  // specified input and adds it to the view.
  // NOTE: The returned representation pointer is not reference-counted, 
  // so you MUST call Register() on the representation if you want to 
  // keep a reference to it.
  vtkDataRepresentation* AddRepresentationFromInput(vtkDataObject* input);

  // Description:
  // Convenience method which sets the representation to the 
  // specified input and adds it to the view.
  // NOTE: The returned representation pointer is not reference-counted, 
  // so you MUST call Register() on the representation if you want to 
  // keep a reference to it.
  vtkDataRepresentation* SetRepresentationFromInput(vtkDataObject* input);

  // Description:
  // Removes the representation from the view.
  void RemoveRepresentation(vtkDataRepresentation* rep);

  // Description:
  // Removes any representation with this connection from the view.
  void RemoveRepresentation(vtkAlgorithmOutput* rep);
  
  // Description:
  // Removes all representations from the view.
  void RemoveAllRepresentations(); 

  // Description:
  // Returns the number of representations from first port(0) in this view.
  int GetNumberOfRepresentations();

  // Description:
  // The representation at a specified index.
  vtkDataRepresentation* GetRepresentation(int index = 0);

  // Description:
  // Check to see if a representation is present in the view.
  bool IsRepresentationPresent(vtkDataRepresentation* rep);

  // Description:
  // Update the view.
  virtual void Update();
  
  // Description:
  // Apply a theme to the view.
  virtual void ApplyViewTheme(vtkViewTheme* vtkNotUsed(theme)) { }

  //BTX
  // Description:
  // Returns the observer that the subclasses can use to listen to additional
  // events. Additionally these subclasses should override
  // ProcessEvents() to handle these events.
  vtkCommand* GetObserver();

  // Description:
  // A ptr to an instance of ViewProgressEventCallData is provided in the call
  // data when vtkCommand::ViewProgressEvent is fired.
  class ViewProgressEventCallData
    {
    const char* Message;
    double Progress;

  public:
    ViewProgressEventCallData(const char* msg, double progress)
      {
      this->Message = msg;
      this->Progress = progress;
      }
    ~ViewProgressEventCallData()
      {
      this->Message = 0;
      }

    // Description:
    // Get the message.
    const char* GetProgressMessage() const
      { return this->Message; }

    // Description:
    // Get the progress value in range [0.0, 1.0].
    double GetProgress() const
      { return this->Progress; }
    };
  //ETX
  
  // Description:
  // Meant for use by subclasses and vtkRepresentation subclasses.
  // Call this method to register vtkObjects (generally
  // vtkAlgorithm subclasses) which fire vtkCommand::ProgressEvent with the
  // view. The view listens to vtkCommand::ProgressEvent and fires
  // ViewProgressEvent with ViewProgressEventCallData containing the message and
  // the progress amount. If message is not provided, then the class name for
  // the algorithm is used.
  void RegisterProgress(vtkObject* algorithm, const char* message=NULL);

  // Description:
  // Unregister objects previously registered with RegisterProgress.
  void UnRegisterProgress(vtkObject* algorithm);

  // Description:
  // This is a Update-Data pass. All representations are expected to update
  // their inputs and prepare geometries for rendering. All heavy work that has
  // to happen only when input-data changes can be done in this pass.
  // This is the first pass.
  static vtkInformationRequestKey* REQUEST_UPDATE();

  // Description:
  // This is a Request-MetaData pass. This happens only after REQUEST_UPDATE()
  // has happened. In this pass representations typically publish information
  // that may be useful for rendering optimizations such as geometry sizes, etc.
  static vtkInformationRequestKey* REQUEST_INFORMATION();

  // Description:
  // This is a Prepare-for-rendering pass. This happens only after
  // REQUEST_UPDATE() has happened. This is called for every render.
  static vtkInformationRequestKey* REQUEST_PREPARE_FOR_RENDER();

  // Description:
  // This is a render pass. This happens only after
  // REQUEST_PREPARE_FOR_RENDER() has happened. This is called for every render.
  static vtkInformationRequestKey* REQUEST_RENDER();

//BTX
protected:
  vtkView();
  ~vtkView();
  
  // Description:
  // Called to process events.
  // The superclass processes selection changed events from its representations.
  // This may be overridden by subclasses to process additional events.
  virtual void ProcessEvents(vtkObject* caller, unsigned long eventId, 
    void* callData);
    
  // Description:
  // Connects to the internal pipeline.
  // Subclasses that handle tight integration between view and
  // representation should override this method.
  virtual void AddInputConnection(
    vtkAlgorithmOutput* vtkNotUsed(conn),
    vtkAlgorithmOutput* vtkNotUsed(selectionConn)) { }
  
  // Description:
  // Disconnects the internal pipeline.
  // Subclasses that handle tight integration between view and
  // representation should override this method.
  virtual void RemoveInputConnection(
    vtkAlgorithmOutput* vtkNotUsed(conn),
    vtkAlgorithmOutput* vtkNotUsed(selectionConn)) { }
  
  // Description:
  // Create a default vtkDataRepresentation for the given vtkAlgorithmOutput.
  // View subclasses may override this method to create custom representations.
  // This method is called by Add/SetRepresentationFromInputConnection.
  // NOTE, the caller must delete the returned vtkDataRepresentation.
  virtual vtkDataRepresentation* CreateDefaultRepresentation(vtkAlgorithmOutput* conn);

  // Description:
  // Subclass "hooks" for notifying subclasses of vtkView when representations are added
  // or removed. Override these methods to perform custom actions.
  virtual void AddRepresentationInternal(vtkDataRepresentation* vtkNotUsed(rep)) {}
  virtual void RemoveRepresentationInternal(vtkDataRepresentation* vtkNotUsed(rep)) {}

  // Description:
  // True if the view takes a single representation that should be reused on
  // Add/SetRepresentationFromInput(Connection) calls. Default is off.
  vtkSetMacro(ReuseSingleRepresentation, bool);
  vtkGetMacro(ReuseSingleRepresentation, bool);
  vtkBooleanMacro(ReuseSingleRepresentation, bool);
  bool ReuseSingleRepresentation;

  // Description:
  // These are passed as arguments to
  // vtkDataRepresentation::ProcessViewRequest(). This avoid repeated creation
  // and deletion of vtkInformation objects.
  vtkInformation* RequestInformation;
  vtkInformationVector* ReplyInformationVector;

  // Description:
  // Subclasses can use this method to trigger a pass on all representations.
  void CallProcessViewRequest(
    vtkInformationRequestKey* passType,
    vtkInformation* request, vtkInformationVector* reply);

private:
  vtkView(const vtkView&);  // Not implemented.
  void operator=(const vtkView&);  // Not implemented.

  class vtkImplementation;
  vtkImplementation* Implementation;

  class Command;
  friend class Command;
  Command* Observer;

  class vtkInternal;
  vtkInternal* Internal;
//ETX
};

#endif
