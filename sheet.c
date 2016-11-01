#include "bootpack.h"
#include <stdio.h>

struct SHTCTL *shtctl_init(struct MEMMAN *mem, unsigned char *vram, int xsize, int ysize){
	struct SHTCTL *ctl;
	int i;
	
	ctl = (struct SHTCTL *) memory_alloc_4k(mem, sizeof (struct SHTCTL));
	if(ctl == 0){
		return ctl;
	}
	ctl->vram = vram;
	ctl->xsize = xsize;
	ctl->ysize = ysize;
	ctl->top = -1;  //一个SHEET都没有
	for(i = 0; i < MAX_SHEETS; ++i){
		ctl->sheets0[i].flags = 0; //均标记为未使用
		ctl->sheets0[i].ctl = ctl;
	}
	return ctl;
}
struct  SHEET *sheet_alloc(struct SHTCTL *ctl){
	struct SHEET *sht;
	int i;
	for(i = 0; i < MAX_SHEETS; ++i){
		if(ctl->sheets0[i].flags == 0){
			sht = &(ctl->sheets0[i]);
			sht->flags = 1; //将此图层标记为正在使用
			sht->height = -1; //先隐藏，一会儿使用
			return sht;
		}
	}
	return 0;  //无可使用的SHEET
}
void sheet_set_buf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv){
	sht->buf = buf;
	sht->bxsize = xsize;
	sht->bysize = ysize;
	sht->col_inv = col_inv;
	return;
}
void sheet_set_height(struct SHEET *sht, int height){
	struct SHTCTL *ctl = sht->ctl;
	int h, old = sht->height;
	//对height进行修正
	if(height > ctl->top + 1){
		height = ctl->top + 1;
	}
	if(height < -1){
		height = -1;
	}
	sht->height = height; //设定给定高度
	
	//对sheet[]重新排列
	if(height < old){ //比以前低
		if(height >= 0){ //之间的SHEET上升
			for(h = old; h > height; --h){
				ctl->sheets[h] = ctl->sheets[h - 1];
				ctl->sheets[h]->height = h;
			}
			ctl->sheets[height] = sht;
			sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1);
		}
		else{ //height == -1, 隐藏sht, sht上面的降下来
			if(ctl->top > old){
				for(h = old; h < ctl->top; ++h){
					ctl->sheets[h] = ctl->sheets[h + 1];
					ctl->sheets[h]->height = h;
				}
			}
			ctl->top--; //少了一个SHEET
			sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0); //刷新画面
		}
		
	}
	else if(height > old){ //比以前高
		if(old >= 0){ //先前就存在，之间的SHEET下降
			for(h = old; h < height; ++h){
				ctl->sheets[h] = ctl->sheets[h + 1];
				ctl->sheets[h]->height = h;
			}
			ctl->sheets[height] = sht;
		}
		else{ //old == -1, 由隐藏变显示, sht后面的升上去
			for(h = ctl->top; h > height; --h){
				ctl->sheets[h + 1] = ctl->sheets[h];
				ctl->sheets[h + 1]->height = h + 1;
			}
			ctl->sheets[height] = sht;
			ctl->top++;  //多了一个SHEET
		}
		sheet_refreshsub(ctl, sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height); //刷新画面
	}
	return;	
}

//画面刷新
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1){
	if(sht->height >= 0){
		sheet_refreshsub(sht->ctl, sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1, sht->height);
	}
	return;
}

void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1, int h0){
	int h, vx, vy, bx, by, bx0, bx1, by0, by1;
	unsigned char *buf, c, *vram;
	vram = ctl->vram;
	struct SHEET *sht;
	if(vx0 < 0){vx0 = 0;}
	if(vy0 < 0){vy0 = 0;}
	if(vx1 > ctl->xsize){vx1 = ctl->xsize;}
	if(vy1 > ctl->ysize){vy1 = ctl->ysize;}
	for(h = h0; h <= ctl->top; ++h){
		sht = ctl->sheets[h];
		buf = sht->buf;
		bx0 = vx0 - sht->vx0;
		bx1 = vx1 - sht->vx0;
		by0 = vy0 - sht->vy0;
		by1 = vy1 - sht->vy0;
		if(bx0 < 0){bx0 = 0;} //健壮性更强
		if(by0 < 0){by0 = 0;} //使得叠加区域的处理更完善
		if(bx1 > sht->bxsize){bx1 = sht->bxsize;}
		if(by1 > sht->bysize){by1 = sht->bysize;}
		for(by = by0; by < by1; ++by){
			vy = sht->vy0 + by;
			for(bx = bx0; bx < bx1; ++bx){
				vx = sht->vx0 + bx;
				c = buf[by * sht->bxsize + bx];
				if(c != sht->col_inv){ //如果颜色不是透明就显示
					vram[vy * ctl->xsize + vx] = c;
				}
			}
		}
	}
	return;
}

void sheet_slide(struct SHEET *sht, int vx0, int vy0){
	int old_vx0 = sht->vx0, old_vy0 = sht->vy0;
	sht->vx0 = vx0;
	sht->vy0 = vy0;
	if(sht->height >= 0){
		sheet_refreshsub(sht->ctl, old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize,0);
		sheet_refreshsub(sht->ctl, vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height);
	}
	return;
}

void sheet_free(struct SHEET *sht){
	if(sht->height >= 0){
		sheet_set_height(sht, -1);
	}
	sht->flags = 0;
	return;
}







