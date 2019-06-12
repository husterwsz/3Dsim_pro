/*****************************************************************************************************************************
This is a project on 3Dsim, based on ssdsim under the framework of the completion of structures, the main function:
1.Support for 3D commands, for example:mutli plane\interleave\copyback\program suspend/Resume..etc
2.Multi - level parallel simulation
3.Clear hierarchical interface
4.4-layer structure

FileName£º flash.c
Author: Zuo Lu 		Version: 2.0	Date:2017/02/07
Description:
flash layer: the original ssdsim this layer is not a specific description, it was their own package to achieve, not completed.

History:
<contributor>		<time>			<version>       <desc>													<e-mail>
Zuo Lu				2017/04/06	      1.0		    Creat 3Dsim											lzuo@hust.edu.cn
Zuo Lu				2017/05/12		  1.1			Support advanced commands:mutli plane					lzuo@hust.edu.cn
Zuo Lu				2017/06/12		  1.2			Support advanced commands:half page read				lzuo@hust.edu.cn
Zuo Lu				2017/06/16		  1.3			Support advanced commands:one shot program				lzuo@hust.edu.cn
Zuo Lu				2017/06/22		  1.4			Support advanced commands:one shot read					lzuo@hust.edu.cn
Zuo Lu				2017/07/07		  1.5			Support advanced commands:erase suspend/resume			lzuo@hust.edu.cn
Zuo Lu				2017/07/24		  1.6			Support static allocation strategy						lzuo@hust.edu.cn
Zuo Lu				2017/07/27		  1.7			Support hybrid allocation strategy						lzuo@hust.edu.cn
Zuo Lu				2017/08/17		  1.8			Support dynamic stripe allocation strategy				lzuo@hust.edu.cn
Zuo Lu				2017/10/11		  1.9			Support dynamic OSPA allocation strategy				lzuo@hust.edu.cn
Jin Li				2018/02/02		  1.91			Add the allocation_method								li1109@hust.edu.cn
Ke wang/Ke Peng		2018/02/05		  1.92			Add the warmflash opsration								296574397@qq.com/2392548402@qq.com
Hao Lv				2018/02/06		  1.93			Solve gc operation bug 									511711381@qq.com
Zuo Lu				2018/02/07        2.0			The release version 									lzuo@hust.edu.cn
***********************************************************************************************************************************************/

#define _CRTDBG_MAP_ALLOC

#include <stdlib.h>
#include <crtdbg.h>

#include "initialize.h"
#include "ssd.h"
#include "flash.h"
#include "buffer.h"
#include "interface.h"
#include "ftl.h"
#include "fcl.h"


/******************************************************************************************
*function is to erase the operation, the channel, chip, die, plane under the block erase
*******************************************************************************************/
Status erase_operation(struct ssd_info * ssd, unsigned int channel, unsigned int chip, unsigned int die, unsigned int plane, unsigned int block)
{
	unsigned int i = 0;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].plane_erase_count++;

	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].free_page_num = ssd->parameter->page_block;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].invalid_page_num = 0;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_write_page = -1;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].erase_count++;

	for (i = 0; i<ssd->parameter->page_block; i++)
	{
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].free_state = PG_SUB;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].valid_state = 0;
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[i].lpn = -1;
	}
	ssd->erase_count++;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page += ssd->parameter->page_block;

	return SUCCESS;

}

/******************************************************************************************
*function is to read out the old active page, set invalid, migrate to the new valid page
*******************************************************************************************/
Status move_page(struct ssd_info * ssd, struct local *location, unsigned int move_plane,unsigned int * transfer_size)
{
	return SUCCESS;
}

Status  NAND_read(struct ssd_info *ssd, struct sub_request * req)
{
	unsigned int chan, chip, die, plane, block, page;
	chan = req->location->channel;
	chip = req->location->chip;
	die = req->location->die;
	plane = req->location->plane;
	block = req->location->block;
	page = req->location->page;

	if (ssd->channel_head[chan].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].block_type == DATA_BLK)
	{
		ssd->data_read_cnt++;
		if (req->read_flag == UPDATE_READ)
			ssd->data_update_cnt++;
		else
			ssd->data_req_cnt++;
	}
	else
	{
		ssd->tran_read_cnt++;
		if (req->read_flag == UPDATE_READ)
			ssd->tran_update_cnt++;
		else
			ssd->tran_req_cnt++;
	}
		
	if (ssd->channel_head[chan].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page].lpn != req->lpn)
		return FAILURE;
	ssd->read_count++;
	return SUCCESS;
}


Status  NAND_program(struct ssd_info *ssd, struct sub_request * req)
{
	unsigned int chan, chip, die, plane, block, page;
	chan  = req->location->channel;
	chip  = req->location->chip;
	die   = req->location->die;
	plane = req->location->plane;
	block = req->location->block;
	page  = req->location->page;

	ssd->channel_head[chan].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].page_head[page].lpn = req->lpn;
	ssd->channel_head[chan].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_write_page++;  //ilitialization is -1
	if (ssd->channel_head[chan].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].last_write_page != page)
		return FAILURE;

	if (ssd->channel_head[chan].chip_head[chip].die_head[die].plane_head[plane].blk_head[block].block_type == DATA_BLK)
	{
		ssd->data_program_cnt++;
	}
	else
	{
		ssd->tran_program_cnt++;
	}
   
	ssd->write_flash_count++;
	ssd->program_count++;
	return SUCCESS;
}


/*********************************************************************************************
*this function is a simulation of a real write operation, to the pre-processing time to use
*********************************************************************************************/
Status write_page(struct ssd_info *ssd, unsigned int channel, unsigned int chip, unsigned int die, unsigned int plane, unsigned int active_block, unsigned int *ppn)
{
	int last_write_page = 0;
	last_write_page = ++(ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page);
	if (last_write_page >= (int)(ssd->parameter->page_block))
	{
		ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].last_write_page = 0;
		printf("error! the last write page larger than max!!\n");
		return ERROR;
	}

	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].free_page_num--;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].free_page--;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].blk_head[active_block].page_head[last_write_page].written_count++;
	ssd->write_flash_count++;
	ssd->channel_head[channel].chip_head[chip].die_head[die].plane_head[plane].test_pre_count++;
	
	*ppn = find_ppn(ssd, channel, chip, die, plane, active_block, last_write_page);
	
	return SUCCESS;
}

/***********************************************************************************************************
*function is to modify the page page to find the state and the corresponding dram in the mapping table value
***********************************************************************************************************/
struct ssd_info *flash_page_state_modify(struct ssd_info *ssd, struct sub_request *sub, unsigned int channel, unsigned int chip, unsigned int die, unsigned int plane, unsigned int block, unsigned int page)
{
	unsigned int ppn, full_page;
	struct local *location;
	struct direct_erase *new_direct_erase, *direct_erase_node;

	return ssd;
}

