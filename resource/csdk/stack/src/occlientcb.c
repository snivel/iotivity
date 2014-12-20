//******************************************************************
//
// Copyright 2014 Intel Mobile Communications GmbH All Rights Reserved.
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


#include "occlientcb.h"
#include "occoap.h"
#include "utlist.h"
#include "logger.h"
#include "ocmalloc.h"
#include <string.h>

#ifdef CA_INT
    #include "cacommon.h"
    #include "cainterface.h"
#endif

/// Module Name
#define TAG PCF("occlientcb")

struct ClientCB *cbList = NULL;
OCMulticastNode * mcPresenceNodes = NULL;

OCStackResult
#ifdef CA_INT
AddClientCB (ClientCB** clientCB, OCCallbackData* cbData,
             CAToken_t * token, OCDoHandle *handle, OCMethod method,
             unsigned char * requestUri, unsigned char * resourceTypeName)
#else // CA_INT
AddClientCB (ClientCB** clientCB, OCCallbackData* cbData,
             OCCoAPToken * token, OCDoHandle *handle, OCMethod method,
             unsigned char * requestUri, unsigned char * resourceTypeName)
#endif // CA_INT
{

    ClientCB *cbNode = NULL;

    #ifdef WITH_PRESENCE
    if(method == OC_REST_PRESENCE)
    {   // Retrieve the presence callback structure for this specific requestUri.
        cbNode = GetClientCB(NULL, NULL, requestUri);
    }
    #endif // WITH_PRESENCE

    if(!cbNode)// If it does not already exist, create new node.
    {
        cbNode = (ClientCB*) OCMalloc(sizeof(ClientCB));
        if(!cbNode)
        {
            *clientCB = NULL;
            goto exit;
        }
        else
        {
            cbNode->callBack = cbData->cb;
            cbNode->context = cbData->context;
            cbNode->deleteCallback = cbData->cd;
            #ifdef CA_INT
            cbNode->token = (CAToken_t *)OCMalloc(CA_MAX_TOKEN_LEN);
            memcpy(&(cbNode->token), token, CA_MAX_TOKEN_LEN);
            #else // CA_INT
            memcpy(&(cbNode->token), token, sizeof(OCCoAPToken));
            #endif // CA_INT
            cbNode->handle = *handle;
            cbNode->method = method;
            cbNode->sequenceNumber = 0;
            #ifdef WITH_PRESENCE
            cbNode->presence = NULL;
            cbNode->filterResourceType = NULL;
            #endif // WITH_PRESENCE
            cbNode->requestUri = requestUri;
            LL_APPEND(cbList, cbNode);
            *clientCB = cbNode;
        }
    }
    else
    {
        // Ensure that the handle the SDK hands back up to the application layer for the
        // OCDoResource call matches the found ClientCB Node.
        *clientCB = cbNode;
        OCFree(requestUri);
        OCFree(*handle);
        *handle = cbNode->handle;
    }

    #ifdef WITH_PRESENCE
    if(method == OC_REST_PRESENCE && resourceTypeName)
    {   // Amend the found or created node by adding a new resourceType to it.
        return InsertResourceTypeFilter(cbNode, (const char *)resourceTypeName);
    }
    #endif

    return OC_STACK_OK;

    exit:
        return OC_STACK_NO_MEMORY;
}

void DeleteClientCB(ClientCB * cbNode) {
    if(cbNode) {
        LL_DELETE(cbList, cbNode);
        OC_LOG(INFO, TAG, PCF("deleting tokens"));
        #ifdef CA_INT
        CADestroyToken (cbNode->token);
        #else
        OC_LOG_BUFFER(INFO, TAG, cbNode->token.token, cbNode->token.tokenLength);
        #endif // CA_INT
        OCFree(cbNode->handle);
        OCFree(cbNode->requestUri);
        if(cbNode->deleteCallback)
        {
            cbNode->deleteCallback(cbNode->context);
        }

        #ifdef WITH_PRESENCE
        if(cbNode->presence) {
            OCFree(cbNode->presence->timeOut);
            OCFree(cbNode->presence);
        }
        if(cbNode->method == OC_REST_PRESENCE)
        {
            OCResourceType * pointer = cbNode->filterResourceType;
            OCResourceType * next = NULL;
            while(pointer)
            {
                next = pointer->next;
                OCFree(pointer->resourcetypename);
                OCFree(pointer);
                pointer = next;
            }
        }
        #endif // WITH_PRESENCE
        OCFree(cbNode);
        cbNode = NULL;
    }
}

#ifdef CA_INT
ClientCB* GetClientCB(CAToken_t * token, OCDoHandle handle, const unsigned char * requestUri)
#else // CA_INT
ClientCB* GetClientCB(OCCoAPToken * token, OCDoHandle handle, const unsigned char * requestUri)
#endif // CA_INT
{
    ClientCB* out = NULL;
    if(token) {
        LL_FOREACH(cbList, out) {
            OC_LOG(INFO, TAG, PCF("comparing tokens"));
            #ifdef CA_INT
            if(memcmp(out->token, *token, CA_MAX_TOKEN_LEN) == 0)
            {
                return out;
            }
            #else // CA_INT
            OC_LOG_BUFFER(INFO, TAG, token->token, token->tokenLength);
            OC_LOG_BUFFER(INFO, TAG, out->token.token, out->token.tokenLength);
            if((out->token.tokenLength == token->tokenLength) &&
                (memcmp(out->token.token, token->token, token->tokenLength) == 0) ) {
                return out;
            }
            #endif // CA_INT
        }
    }
    else if(handle) {
        LL_FOREACH(cbList, out) {
            if(out->handle == handle) {
                return out;
            }
        }
    }
    else if(requestUri) {
        LL_FOREACH(cbList, out) {
            if(out->requestUri && strcmp((char *)out->requestUri, (char *)requestUri) == 0) {
                return out;
            }
        }
    }
    OC_LOG(INFO, TAG, PCF("Callback Not found !!"));
    return NULL;
}

#ifdef WITH_PRESENCE
OCStackResult InsertResourceTypeFilter(ClientCB * cbNode, const char * resourceTypeName)
{
    OCResourceType * newResourceType = NULL;
    if(cbNode && resourceTypeName)
    {
        // Form a new resourceType member.
        newResourceType = (OCResourceType *) OCMalloc(sizeof(OCResourceType));
        if(!newResourceType)
        {
            return OC_STACK_NO_MEMORY;
        }

        newResourceType->next = NULL;
        newResourceType->resourcetypename = (char *) resourceTypeName;

        LL_APPEND(cbNode->filterResourceType, newResourceType);
        return OC_STACK_OK;
    }
    return OC_STACK_ERROR;
}
#endif // WITH_PRESENCE

void DeleteClientCBList() {
    ClientCB* out;
    ClientCB* tmp;
    LL_FOREACH_SAFE(cbList, out, tmp) {
        DeleteClientCB(out);
    }
    cbList = NULL;
}

void FindAndDeleteClientCB(ClientCB * cbNode) {
    ClientCB* tmp;
    if(cbNode)
    {
        LL_FOREACH(cbList, tmp)
        {
            if (cbNode == tmp)
            {
                DeleteClientCB(tmp);
                break;
            }
        }
    }
}

OCStackResult AddMCPresenceNode(OCMulticastNode** outnode, unsigned char* uri, uint32_t nonce)
{
    OCMulticastNode *node;

    node = (OCMulticastNode*) OCMalloc(sizeof(OCMulticastNode));

    if (node) {
        node->nonce = nonce;
        node->uri = uri;
        LL_APPEND(mcPresenceNodes, node);
        *outnode = node;
        return OC_STACK_OK;
    }
    *outnode = NULL;
    return OC_STACK_NO_MEMORY;
}

OCMulticastNode* GetMCPresenceNode(const unsigned char * uri) {
    OCMulticastNode* out = NULL;

    if(uri) {
        LL_FOREACH(mcPresenceNodes, out) {
            if(out->uri && strcmp((char *)out->uri, (char *)uri) == 0) {
                return out;
            }
        }
    }
    OC_LOG(INFO, TAG, PCF("MulticastNode Not found !!"));
    return NULL;
}
