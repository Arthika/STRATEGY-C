/*--------------------------------------------------------------------------------------------------
-- Project     :    remoteStub
-- Filename    :    name: remoteStubSo.h
--                  created_by: carlos
--                  date_created: Nov 24, 2014
--------------------------------------------------------------------------------------------------
-- File Purpose: Exported functions for remote stub dinamic library
-- to be called from System Manager
--------------------------------------------------------------------------------------------------
-- This software is protected by copyright, the design of any article
-- recorded in the software is protected by design right and the
-- information contained in the software is confidential. This software
-- may not be copied, any design may not be reproduced and the information
-- contained in the software may not be used or disclosed except with the
-- prior written permission of and in a manner permitted by the proprietors
-- Arthika Trading Solutions, S.L.
--------------------------------------------------------------------------------------------------
-- Changes log:
-- 
--------------------------------------------------------------------------------------------------*/

#ifndef REMOTESTUBSO_H_
#define REMOTESTUBSO_H_

extern int remoteStubStart(void);

extern int32 internalStubSendto (byte* buffer, int32 len);

#endif /* REMOTESTUBSO_H_ */
