//******************************************************************
//
// Copyright 2015 Samsung Electronics All Rights Reserved.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

///
/// This sample provides steps to define an interface for a resource
/// (properties and methods) and host this resource on the server.
///

#include <functional>

#include <pthread.h>
#include <mutex>
#include <string>
#include <condition_variable>

#include "OCPlatform.h"
#include "OCApi.h"
#include <Elementary.h>

using namespace OC;
namespace PH = std::placeholders;

int gObservation = 0;
bool isListOfObservers = false;
void * ChangeLightRepresentation(void *param);
Evas_Object *images[3][3];

typedef enum {
    ON,
    ALARM,
    MORNING,
    EVENING,
    NIGHT,
    OFF,
} bulbstate;

const char *bulbfile[] = {
    "on.png",
    "bulb.png",
    "orange.png",
    "blue.png",
    "violet.png",
    "off.png",
};

int states[3][3];

void change_bg_image(bulbstate state) {
    char buf[PATH_MAX];
    sprintf(buf, "images/%s", bulbfile[state]);
	int r, c;

	do
	{
    	r = (rand() * 10)%3;
    	c = (rand() * 10)%3;
	}while(states[r][c]==state);

	printf("Changing bulb state of [%d][%d] = %s\n", r, c, bulbfile[state]);
    elm_photo_file_set(images[r][c], buf);
	states[r][c] = state;
}

void
my_win_del(void *data EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED) {
    elm_exit(); /* exit the program's main loop that runs in elm_run() */
}

void
launch_bulb() {
    Evas_Object *win, *bg;
    Evas_Object *box;
    char buf[PATH_MAX];

    win = elm_win_add(NULL, "bg-image", ELM_WIN_BASIC);
    elm_win_title_set(win, "WSI Demo Control");
    elm_win_autodel_set(win, EINA_TRUE);
    evas_object_smart_callback_add(win, "delete,request", my_win_del, NULL);

    bg = elm_bg_add(win);
    evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(win, bg);
    evas_object_show(bg);

    box = elm_box_add(win);
    evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(win, box);
    evas_object_show(box);

   Evas_Object *tb = elm_table_add(win);
   evas_object_size_hint_weight_set(tb, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   

   int n = 0;
   Evas_Object *ph;
   for (int j = 0; j < 3; j++)
     {
        for (int i = 0; i < 3; i++)
          {
             ph = elm_photo_add(win);
             snprintf(buf, sizeof(buf), "images/%s",bulbfile[n]);
             n++;
             if (n >= 5) n = 0;
             images[j][i] = ph;
             elm_photo_aspect_fixed_set(ph, EINA_FALSE);
             elm_photo_size_set(ph, 80);
             elm_photo_file_set(ph, buf);
             elm_photo_editable_set(ph, EINA_TRUE);
             evas_object_size_hint_weight_set(ph, EVAS_HINT_EXPAND,EVAS_HINT_EXPAND);
             evas_object_size_hint_align_set(ph, EVAS_HINT_FILL, EVAS_HINT_FILL);
             elm_table_pack(tb, ph, i, j, 1, 1);
             evas_object_show(ph);
          }
     }
    elm_box_pack_end(box, tb);
    evas_object_show(tb);
    evas_object_size_hint_min_set(bg, 160, 160);
    evas_object_size_hint_max_set(bg, 640, 640);
    evas_object_resize(win, 320, 320);
    evas_object_show(win);
}

/// This class represents a single resource named 'lightResource'. This resource has
/// two simple properties named 'state' and 'power'

class LightResource
{

public:
    /// Access this property from a TB client
    std::string m_name;
    int m_state;
    std::string m_lightUri;
    OCResourceHandle m_resourceHandle;
    OCRepresentation m_lightRep;
    ObservationIds m_interestedObservers;

public:
    /// Constructor
    LightResource(PlatformConfig& /*cfg*/)
        :m_name("OIC Light"), m_state(0), m_lightUri("/a/light") {
        // Initialize representation
        m_lightRep.setUri(m_lightUri);

        m_lightRep.setValue("state", m_state);
        m_lightRep.setValue("name", m_name);
    }

    /* Note that this does not need to be a member function: for classes you do not have
    access to, you can accomplish this with a free function: */

    /// This function internally calls registerResource API.
    void createResource()
    {
        std::string resourceURI = m_lightUri; // URI of the resource
        // resource type name. In this case, it is light
        std::string resourceTypeName = "core.light";
        std::string resourceInterface = DEFAULT_INTERFACE; // resource interface.

        // OCResourceProperty is defined ocstack.h
        uint8_t resourceProperty = OC_DISCOVERABLE | OC_OBSERVABLE;

        EntityHandler cb = std::bind(&LightResource::entityHandler, this,PH::_1);

        // This will internally create and register the resource.
        OCStackResult result = OCPlatform::registerResource(
                                    m_resourceHandle, resourceURI, resourceTypeName,
                                    resourceInterface, cb, resourceProperty);

        if (OC_STACK_OK != result)
        {
		std::cout << "Resource creation was unsuccessful\n";
        }
    }

    OCResourceHandle getHandle()
    {
        return m_resourceHandle;
    }

    // Post representation.
    // Post can create new resource or simply act like put.
    // Gets values from the representation and
    // updates the internal state
    OCRepresentation post(OCRepresentation& rep)
    {
	    std::cout << "Post incoked......................................." << std::endl;
        try
        {
            if (rep.getValue("state", m_state)) {
		    std::cout << "\t\t\t\t" << "----state: " << m_state << std::endl;
                change_bg_image(m_state);
            } else {
		    std::cout << "\t\t\t\t" << "state not found in the representation" << std::endl;
            }
        }

        catch(std::exception & e) {
		std::cout << e.what() << std::endl;
        }
        return get();
    }


    // gets the updated representation.
    // Updates the representation with latest internal state before
    // sending out.
    OCRepresentation get()
    {
        m_lightRep.setValue("state", m_state);
        return m_lightRep;
    }

    void addType(const std::string& type) const
    {
        OCStackResult result = OCPlatform::bindTypeToResource(m_resourceHandle, type);
        if (OC_STACK_OK != result)
        {
		std::cout << "Binding TypeName to Resource was unsuccessful\n";
        }
    }

    void addInterface(const std::string& interface) const
    {
        OCStackResult result = OCPlatform::bindInterfaceToResource(m_resourceHandle, interface);
        if (OC_STACK_OK != result)
        {
		std::cout << "Binding TypeName to Resource was unsuccessful\n";
        }
    }

private:

OCStackResult sendResponse(std::shared_ptr<OCResourceRequest> pRequest)
{
    auto pResponse = std::make_shared<OC::OCResourceResponse>();
    pResponse->setRequestHandle(pRequest->getRequestHandle());
    pResponse->setResourceHandle(pRequest->getResourceHandle());
    pResponse->setResourceRepresentation(get());
    pResponse->setErrorCode(200);
    pResponse->setResponseResult(OC_EH_OK);

    return OCPlatform::sendResponse(pResponse);
}

OCStackResult sendPostResponse(std::shared_ptr<OCResourceRequest> pRequest)
{
    auto pResponse = std::make_shared<OC::OCResourceResponse>();
    pResponse->setRequestHandle(pRequest->getRequestHandle());
    pResponse->setResourceHandle(pRequest->getResourceHandle());

    OCRepresentation rep = pRequest->getResourceRepresentation();
    OCRepresentation rep_post = post(rep);

    pResponse->setResourceRepresentation(rep_post);
    pResponse->setErrorCode(200);
    pResponse->setResponseResult(OC_EH_OK);

    return OCPlatform::sendResponse(pResponse);
}

// This is just a sample implementation of entity handler.
// Entity handler can be implemented in several ways by the manufacturer
OCEntityHandlerResult entityHandler(std::shared_ptr<OCResourceRequest> request)
{
	std::cout << "\tIn Server CPP entity handler:\n";
        OCEntityHandlerResult ehResult = OC_EH_ERROR;
        if (request) {
            std::string requestType = request->getRequestType();
            int requestFlag = request->getRequestHandlerFlag();
            if (requestFlag & RequestHandlerFlag::RequestFlag) {
		    std::cout << "\t\trequestFlag : Request\n";
                auto pResponse = std::make_shared<OC::OCResourceResponse>();
                pResponse->setRequestHandle(request->getRequestHandle());
                pResponse->setResourceHandle(request->getResourceHandle());
                QueryParamsMap queries = request->getQueryParameters();
                if (!queries.empty()) {
                    std::cout << "\nQuery processing upto entityHandler" << std::endl;
                }
                for (auto it : queries) {
                    std::cout << "Query key: " << it.first << " value : " << it.second
                            << std::endl;
                }
                if (requestType == "GET") {
			std::cout << "\t\t\trequestType : GET\n";
                    pResponse->setErrorCode(200);
                    pResponse->setResponseResult(OC_EH_OK);
                    pResponse->setResourceRepresentation(get());
                    if (OC_STACK_OK == OCPlatform::sendResponse(pResponse)) {
                        ehResult = OC_EH_OK;
                    }
                } else if (requestType == "POST") {
			std::cout << "\t\t\trequestType : POST\n";

                    OCRepresentation rep = request->getResourceRepresentation();
                    OCRepresentation rep_post = post(rep);
                    pResponse->setResourceRepresentation(rep_post);
                    pResponse->setErrorCode(200);
                    if (rep_post.hasAttribute("createduri")) {
                        pResponse->setResponseResult(OC_EH_RESOURCE_CREATED);
                        pResponse->setNewResourceUri(rep_post.getValue<std::string>("createduri"));
                    } else {
                        pResponse->setResponseResult(OC_EH_OK);
                    }

                    if (OC_STACK_OK == OCPlatform::sendResponse(pResponse)) {
                        ehResult = OC_EH_OK;
                    }
                }
            }

            if (requestFlag & RequestHandlerFlag::ObserverFlag) {
                ObservationInfo observationInfo = request->getObservationInfo();
                if (ObserveAction::ObserveRegister == observationInfo.action) {
                    m_interestedObservers.push_back(observationInfo.obsId);
                } else if (ObserveAction::ObserveUnregister == observationInfo.action) {
                    m_interestedObservers.erase(std::remove(
                            m_interestedObservers.begin(),
                            m_interestedObservers.end(),
                            observationInfo.obsId),
                            m_interestedObservers.end());
                }
                pthread_t threadId;
		std::cout << "\t\trequestFlag : Observer\n";
                gObservation = 1;
                static int startedThread = 0;

                // Observation happens on a different thread in ChangeLightRepresentation function.
                // If we have not created the thread already, we will create one here.
                if (!startedThread) {
                    pthread_create(&threadId, NULL, ChangeLightRepresentation, (void *) this);
                    startedThread = 1;
                }
                ehResult = OC_EH_OK;
            }
        } else {
            std::cout << "Request invalid" << std::endl;
        }

        return ehResult;
    }

};

// ChangeLightRepresentaion is an observation function,
// which notifies any changes to the resource to stack
// via notifyObservers
void * ChangeLightRepresentation (void *param)
{
    LightResource* lightPtr = (LightResource*) param;

    // This function continuously monitors for the changes
    while (1)
    {
        sleep (3);

        if (gObservation)
        {
            // If under observation if there are any changes to the light resource
            // we call notifyObservors
            //
            // For demostration we are changing the power value and notifying.
            lightPtr->m_state = lightPtr->m_state + 1;
            if (lightPtr->m_state > 6)
                lightPtr->m_state = 0;

	    std::cout << "State updated to : " << lightPtr->m_state << std::endl;
	    std::cout << "Notifying observers with resource handle: " << lightPtr->getHandle() << std::endl;

            OCStackResult result = OC_STACK_OK;

            if(isListOfObservers)
            {
                std::shared_ptr<OCResourceResponse> resourceResponse =
                            std::make_shared<OCResourceResponse>();

                resourceResponse->setErrorCode(200);
                resourceResponse->setResourceRepresentation(lightPtr->get(), DEFAULT_INTERFACE);

                result = OCPlatform::notifyListOfObservers(
                                                            lightPtr->getHandle(),
                                                            lightPtr->m_interestedObservers,
                                                            resourceResponse,
                                                            OC::QualityOfService::HighQos);
            }
            else
            {
                result = OCPlatform::notifyAllObservers(lightPtr->getHandle(),
                                                            OC::QualityOfService::HighQos);
            }

            if(OC_STACK_NO_OBSERVERS == result)
            {
		    std::cout << "No More observers, stopping notifications" << std::endl;
                gObservation = 0;
            }
        }
    }

    return NULL;
}

static FILE* client_open(const char *path, const char *mode) {
    return fopen("./oic_svr_db_server.json", mode);
}

EAPI_MAIN int
elm_main(int argc, char **argv) {
    OCPersistentStorage ps{client_open, fread, fwrite, fclose, unlink};
    PlatformConfig cfg{
        OC::ServiceType::InProc,
        OC::ModeType::Server,
        "0.0.0.0",
        0,
        OC::QualityOfService::LowQos,
        &ps
    };
    OCPlatform::Configure(cfg);
    try{
        LightResource myLight(cfg);
        myLight.createResource();
        std::cout << "Created resource." << std::endl;
        myLight.addType(std::string("core.brightlight"));
        myLight.addInterface(std::string(LINK_INTERFACE));
        std::cout << "Added Interface and Type" << std::endl;
    }

    catch(OCException & e) {
        std::cout << "OCException in main : " << e.what() << std::endl;
    }
    launch_bulb();
    elm_run(); /* and run the program now and handle all events etc. */
    return 0;
}
ELM_MAIN()