/* Copyright (c) 2009 The Khronos Group Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Materials.
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */

/*! \ingroup wfd
 *  \file wfdconfig.c
 *  \brief OpenWF Display SI, configuration module implementation.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "wfdconfig.h"
#include "wfddebug.h"
#include "wfdutils.h"

#include "owfmemory.h"
#include "owfscreen.h"
#include "owfconfig.h"

#define OWF_CONFIGURATION "OpenWFConfiguration"
static WFD_CONFIG* wfd_config = NULL;


static WFDboolean
WFD_Config_InitPortMode(OWF_CONF_GROUP cPm, WFD_PORT_MODE* pm);

static void
WFD_Config_InitPort(OWF_CONF_GROUP cPort, WFD_PORT_CONFIG* port);

static void
WFD_Config_InitPipeline(OWF_CONF_GROUP cPl, WFD_PIPELINE_CONFIG* pl);

static void
WFD_Config_InitPortModes(OWF_CONF_GROUP cPort, WFD_PORT_CONFIG* port);

static void
WFD_Config_InitDisplayData(OWF_CONF_GROUP cPort, WFD_PORT_CONFIG* port);

static WFDint
WFD_Config_InitPorts(OWF_CONF_GROUP cDev, WFD_PORT_CONFIG** ports);


static WFDint
WFD_Config_InitPipelines(OWF_CONF_GROUP cDev, WFD_PIPELINE_CONFIG** pls);

static WFDint
WFD_Config_InitDevices(OWF_CONF_GROUP root, WFD_DEVICE_CONFIG** devices);

static WFD_CONFIG*
WFD_Config_ReadConfig();

static void
WFD_Config_ModuleTerminate();

static void
WFD_Config_ModuleInitialize();



static WFDboolean
WFD_Config_InitPortMode(OWF_CONF_GROUP cPm, WFD_PORT_MODE* pm)
{
    WFDboolean preconfiguredMode;
    OWF_ASSERT(pm);

    preconfiguredMode =
        OWF_Conf_GetElementContenti(cPm, "preconfiguredMode", WFD_FALSE);


    pm->width = OWF_Conf_GetElementContenti(cPm, "width", 0);
    pm->height = OWF_Conf_GetElementContenti(cPm, "height", 0);
    pm->refreshRate = OWF_Conf_GetElementContentf(cPm, "refreshRate", 0.0);
    pm->flipMirrorSupport =
        OWF_Conf_GetElementContenti(cPm, "flipMirrorSupport", WFD_FALSE);
    pm->rotationSupport =
        OWF_Conf_GetElementContenti(cPm, "rotationSupport", WFD_ROTATION_SUPPORT_NONE);
    pm->interlaced = OWF_Conf_GetElementContenti(cPm, "interlaced", WFD_FALSE);

    return preconfiguredMode;
}


static void
WFD_Config_InitPort(OWF_CONF_GROUP cPort, WFD_PORT_CONFIG* port)
{
    OWF_CONF_GROUP grp;
    OWFint i;
    OWF_ASSERT(port);

    port->id = OWF_Conf_GetElementContenti(cPort, "id", WFD_INVALID_PORT_ID);
    port->type = OWF_Conf_GetElementContenti(cPort, "type", WFD_PORT_TYPE_COMPOSITE);
    port->detachable = OWF_Conf_GetElementContenti(cPort, "detachable", WFD_FALSE);
    port->attached = OWF_Conf_GetElementContenti(cPort, "attached", WFD_TRUE);

    grp = OWF_Conf_GetElement(cPort, "nativeResolution");
    port->nativeResolution[0] = OWF_Conf_GetElementContenti(grp, "width", 0);
    port->nativeResolution[1] = OWF_Conf_GetElementContenti(grp, "height", 0);

    grp = OWF_Conf_GetElement(cPort, "physicalSize");
    port->physicalSize[0] = OWF_Conf_GetElementContentf(grp, "width", 0.0);
    port->physicalSize[1] = OWF_Conf_GetElementContentf(grp, "height", 0.0);

    port->fillPortArea = OWF_Conf_GetElementContenti(cPort, "fillPortArea", WFD_TRUE);

    grp = OWF_Conf_GetElement(cPort, "backgroundColor");
    port->backgroundColor[0] = OWF_Conf_GetElementContentf(grp, "red", 0.0);
    port->backgroundColor[1] = OWF_Conf_GetElementContentf(grp, "green", 0.0);
    port->backgroundColor[2] = OWF_Conf_GetElementContentf(grp, "blue", 0.0);

    for (i = 0; i < BG_SIZE; i++)
    {
        if (port->backgroundColor[i] > 1.0)
        {
            port->backgroundColor[i] = 1.0;
        }
        if (port->backgroundColor[i] < 0.0)
        {
            port->backgroundColor[i] = 0.0;
        }
    }

    port->flip = OWF_Conf_GetElementContenti(cPort, "flip", WFD_FALSE);
    port->mirror = OWF_Conf_GetElementContenti(cPort, "mirror", WFD_FALSE);
    port->rotation = OWF_Conf_GetElementContenti(cPort, "rotation", 0);
    port->powerMode = OWF_Conf_GetElementContenti(cPort, "powerMode", WFD_POWER_MODE_OFF);

    grp = OWF_Conf_GetElement(cPort, "gammaRange");
    port->gammaRange[0] = OWF_Conf_GetElementContentf(grp, "min", 1.0);
    port->gammaRange[1] = OWF_Conf_GetElementContentf(grp, "max", 1.0);

    port->gamma = OWF_Conf_GetElementContentf(cPort, "gamma", 1.0);

    port->partialRefreshSupport =
        OWF_Conf_GetElementContenti(cPort, "partialRefreshSupport", WFD_PARTIAL_REFRESH_NONE);

    grp = OWF_Conf_GetElement(cPort, "partialRefreshMaximum");
    port->partialRefreshMaximum[0] = OWF_Conf_GetElementContenti(grp, "width", 0);
    port->partialRefreshMaximum[1] = OWF_Conf_GetElementContenti(grp, "height", 0);

    port->partialRefreshEnable =
        OWF_Conf_GetElementContenti(cPort, "partialRefreshEnable", WFD_PARTIAL_REFRESH_NONE);

    grp = OWF_Conf_GetElement(cPort, "partialRefreshRectangle");
    port->partialRefreshRectangle[0] = OWF_Conf_GetElementContenti(grp, "offsetX", 0);
    port->partialRefreshRectangle[1] = OWF_Conf_GetElementContenti(grp, "offsetY", 0);
    port->partialRefreshRectangle[2] = OWF_Conf_GetElementContenti(grp, "width", 0);
    port->partialRefreshRectangle[3] = OWF_Conf_GetElementContenti(grp, "height", 0);

    grp = OWF_Conf_GetElement(cPort, "bindablePipelineIds");
    port->pipelineIdCount = OWF_Conf_GetNbrElements(grp, "id");
    if (port->pipelineIdCount > 0)
    {
        WFDint i;
        OWF_CONF_ELEMENT el;

        port->pipelineIds = NEW0N(WFDint, port->pipelineIdCount);

        for ( i=0, el = OWF_Conf_GetElement(grp, "id");
              el;
              i++, el = OWF_Conf_GetNextElement(el, "id")
            )
        {
            port->pipelineIds[i] = OWF_Conf_GetContenti(el, WFD_INVALID_PIPELINE_ID);
        }
    }

    port->protectionEnable =
        OWF_Conf_GetElementContenti(cPort, "protectionEnable", WFD_FALSE);
}

static void
WFD_Config_InitDisplayData(OWF_CONF_GROUP cPort, WFD_PORT_CONFIG* port)
{
    OWF_CONF_GROUP grp;
    OWF_ASSERT(port);

    port->displayDataCount = OWF_Conf_GetNbrElements(cPort, "displayData");

    if (port->displayDataCount > 0)
    {
        WFDint i;

        port->displayData =
            NEW0N(WFD_DISPLAY_DATA, port->displayDataCount);

        for ( i = 0, grp = OWF_Conf_GetElement(cPort, "displayData");
              grp;
              i++, grp = OWF_Conf_GetNextElement(grp, "displayData")
            )
        {
            WFDuint8* data;
            char* str;

            port->displayData[i].format =
                OWF_Conf_GetElementContenti(grp, "format", WFD_DISPLAY_DATA_FORMAT_NONE);

            str = OWF_Conf_GetElementContentStr(grp, "data", NULL);
            if (str)
            {
                WFDint dataLen = strlen((char*)str);
                data = NEW0N(WFDuint8, dataLen);
                /* NOTE: base64 to binary conversion would be nice.
                 * Will not be implemented in the SI. In the real world
                 * this data is hw-generated, not config data */
                strncpy((char*)data, str, dataLen);
                port->displayData[i].data = data;
                port->displayData[i].dataSize = dataLen;
                OWF_Conf_FreeContent(str);
            }
        }
    }
}

static void
WFD_Config_InitPortModes(OWF_CONF_GROUP cPort, WFD_PORT_CONFIG* port)
{
    OWF_CONF_GROUP grp;
    OWF_ASSERT(port);

    port->modeCount = OWF_Conf_GetNbrElements(cPort, "portMode");
    if (port->modeCount > 0)
    {
        WFDint i;

        port->modes = NEW0N(WFD_PORT_MODE, port->modeCount);

        for ( i = 0, grp = OWF_Conf_GetElement(cPort, "portMode");
              grp;
              i++, grp = OWF_Conf_GetNextElement(grp, "portMode")
            )
        {
            /* port mode id is made form portId  and sequence number */
            port->modes[i].id = i+1;

            if (WFD_Config_InitPortMode(grp, &port->modes[i]))
            {
                port->preconfiguredMode = port->modes[i].id;
            }
        }
    }
}

static void
WFD_Config_InitPipeline(OWF_CONF_GROUP cPl, WFD_PIPELINE_CONFIG* pl)
{
    OWF_CONF_GROUP grp;
    OWF_ASSERT(pl);

    pl->id = OWF_Conf_GetElementContenti(cPl, "id", WFD_INVALID_PIPELINE_ID);

    /* portId can be set it port is pre-bound to some port */
    pl->portId =
        OWF_Conf_GetElementContenti(cPl, "portId", WFD_INVALID_PORT_ID);

    /* layer is run-time configurable */

    pl->shareable = OWF_Conf_GetElementContenti(cPl, "shareable", WFD_FALSE);
    pl->directRefresh = OWF_Conf_GetElementContenti(cPl, "directRefresh", WFD_FALSE);

    grp = OWF_Conf_GetElement(cPl, "maxSourceSize");
    pl->maxSourceSize[0] = OWF_Conf_GetElementContenti(grp, "width", 0);
    pl->maxSourceSize[1] = OWF_Conf_GetElementContenti(grp, "height", 0);

    grp = OWF_Conf_GetElement(cPl, "sourceRectangle");
    pl->sourceRectangle[0] = OWF_Conf_GetElementContenti(grp, "offsetX", 0);
    pl->sourceRectangle[1] = OWF_Conf_GetElementContenti(grp, "offsetY", 0);
    pl->sourceRectangle[2] = OWF_Conf_GetElementContenti(grp, "width", 0);
    pl->sourceRectangle[3] = OWF_Conf_GetElementContenti(grp, "height", 0);

    pl->flip = OWF_Conf_GetElementContenti(cPl, "flip", WFD_FALSE);
    pl->mirror = OWF_Conf_GetElementContenti(cPl, "mirror", WFD_FALSE);
    pl->rotationSupport =
        OWF_Conf_GetElementContenti(cPl, "rotationSupport", WFD_ROTATION_SUPPORT_NONE);
    pl->rotation = OWF_Conf_GetElementContenti(cPl, "rotation", 0);

    grp = OWF_Conf_GetElement(cPl, "scaleRange");
    pl->scaleRange[0] = OWF_Conf_GetElementContentf(grp, "min", 1.0);
    pl->scaleRange[1] = OWF_Conf_GetElementContentf(grp, "max", 1.0);

    pl->scaleFilter = OWF_Conf_GetElementContenti(cPl, "scaleFilter", WFD_SCALE_FILTER_NONE);

    grp = OWF_Conf_GetElement(cPl, "destinationRectangle");
    pl->destinationRectangle[0] = OWF_Conf_GetElementContenti(grp, "offsetX", 0);
    pl->destinationRectangle[1] = OWF_Conf_GetElementContenti(grp, "offsetY", 0);
    pl->destinationRectangle[2] = OWF_Conf_GetElementContenti(grp, "width", 0);
    pl->destinationRectangle[3] = OWF_Conf_GetElementContenti(grp, "height", 0);

    pl->globalAlpha = OWF_Conf_GetElementContentf(cPl, "globalAlpha", 1.0);

    grp = OWF_Conf_GetElement(cPl, "transparencyFeatures");
    pl->transparencyFeatureCount = OWF_Conf_GetNbrElements(grp, "feature");
    if (pl->transparencyFeatureCount > 0)
    {
        WFDint i;
        OWF_CONF_ELEMENT el;

        pl->transparencyFeatures = NEW0N(WFDbitfield, pl->transparencyFeatureCount);

        for ( i=0, el = OWF_Conf_GetElement(grp, "feature");
              el;
              i++, el = OWF_Conf_GetNextElement(el, "feature")
            )
        {
            pl->transparencyFeatures[i] =
                OWF_Conf_GetContenti(el, WFD_TRANSPARENCY_NONE);
        }
    }


}

WFDint
WFD_Config_InitPorts(OWF_CONF_GROUP cDev, WFD_PORT_CONFIG** ports)
{
    WFDint n;
    OWF_CONF_GROUP cPort;
    WFD_PORT_CONFIG* port;

    /* memory for port configs */
    n = OWF_Conf_GetNbrElements(cDev, "port");
    if (n>0)
    {
        *ports = NEW0N(WFD_PORT_CONFIG, n);
    }

    /* loop through all port configs */
    for ( port = *ports, cPort = OWF_Conf_GetElement(cDev, "port");
          cPort;
          port++, cPort = OWF_Conf_GetNextElement(cPort, "port")
        )
    {
        WFD_Config_InitPort(cPort, port);
        WFD_Config_InitPortModes(cPort, port);
        WFD_Config_InitDisplayData(cPort, port);
    }

    return n;
}


static WFDint
WFD_Config_InitPipelines(OWF_CONF_GROUP cDev, WFD_PIPELINE_CONFIG** pls)
{
    WFDint n;
    OWF_CONF_GROUP cPl;
    WFD_PIPELINE_CONFIG* pl;

    /* memory for pipeline configs */
    n = OWF_Conf_GetNbrElements(cDev, "pipeline");
    if (n > 0)
    {
        *pls = NEW0N(WFD_PIPELINE_CONFIG, n);
    }

    /* loop through all pipeline configs */
    for ( pl = *pls, cPl = OWF_Conf_GetElement(cDev, "pipeline");
          cPl;
          pl++, cPl = OWF_Conf_GetNextElement(cPl, "pipeline")
        )
    {
        WFD_Config_InitPipeline(cPl, pl);
    }

    return n;
}

static WFDint
WFD_Config_InitDevices(OWF_CONF_GROUP root, WFD_DEVICE_CONFIG** devices)
{
    WFDint n;
    WFD_DEVICE_CONFIG* dev;
    OWF_CONF_GROUP cDev;

    /* memory for device configs */
    n =  OWF_Conf_GetNbrElements(root, "device");
    if (n > 0)
    {
        *devices = NEW0N(WFD_DEVICE_CONFIG, n);
    }

    for ( dev = *devices, cDev = OWF_Conf_GetElement(root, "device");
          cDev;
          dev++, cDev = OWF_Conf_GetNextElement(cDev, "device")
        )
    {
        dev->id = OWF_Conf_GetElementContenti(cDev, "id", 0);
        dev->portCount = WFD_Config_InitPorts(cDev, &dev->ports );
        dev->pipelineCount = WFD_Config_InitPipelines(cDev, &dev->pipelines );
    }

    return n;
}


static WFD_CONFIG*
WFD_Config_ReadConfig()
{
    WFD_CONFIG* config = NULL;
    OWF_CONF_DOCUMENT doc = NULL;
    OWF_CONF_GROUP root = NULL;

    config = NEW0(WFD_CONFIG);

    doc = OWF_Conf_GetGetDocument("display_config.xml");
    if (doc)
    {
        root = OWF_Conf_GetRoot(doc, OWF_CONFIGURATION);
    }

    if (root)
    {
        root = OWF_Conf_GetElement(root, "display");
    }

    if (root)
    {
        config->devCount = WFD_Config_InitDevices(root, &config->devices);
    }

    if(doc)
    {
        OWF_Conf_Cleanup(doc);
    }

    return config;
}




static void
WFD_Config_ModuleTerminate()
{
    WFDint i, ii, iii;

    if (!wfd_config)
    {
        return;
    }

    for (i = 0; i < wfd_config->devCount; i++)
    {
        WFD_DEVICE_CONFIG* devConfig = &wfd_config->devices[i];

        for (ii = 0; ii<devConfig->portCount; ii++)
        {
            WFD_PORT_CONFIG* prtConfig = &devConfig->ports[ii];

            xfree(prtConfig->pipelineIds);
            xfree(prtConfig->modes);

            for (iii = 0; iii<prtConfig->displayDataCount; iii++)
            {
                xfree(prtConfig->displayData[iii].data);
            }

            xfree(prtConfig->displayData);
        }

        xfree(devConfig->ports);

        for (ii = 0; ii<devConfig->pipelineCount; ii++)
        {
            WFD_PIPELINE_CONFIG* plConfig = &devConfig->pipelines[ii];

            xfree(plConfig->transparencyFeatures);
        }

        xfree(devConfig->pipelines);
    }

    xfree(wfd_config->devices);
    xfree(wfd_config);
    wfd_config = NULL;

}

static void
WFD_Config_ModuleInitialize()
{
    wfd_config = WFD_Config_ReadConfig();

    OWF_Screen_Initialize();

    atexit(WFD_Config_ModuleTerminate);
}


OWF_API_CALL WFDint OWF_APIENTRY
WFD_Config_GetDevices(WFD_DEVICE_CONFIG** configs) OWF_APIEXIT
{
    if (wfd_config == NULL)
    {
        WFD_Config_ModuleInitialize();
    }

    if (configs && wfd_config)
    {
        *configs = wfd_config->devices;
    }
    else if (configs)
    {
        *configs = NULL;
    }

    return (wfd_config) ? wfd_config->devCount : 0;
}

#ifdef __cplusplus
}
#endif
