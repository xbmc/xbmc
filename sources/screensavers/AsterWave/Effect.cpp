#include "Effect.h"

void AnimationEffect::init(WaterSettings * settings)
{
  m_pSettings = settings;

  scalex = (m_pSettings->waterField->xMax()-m_pSettings->waterField->xMin());
  scaley = (m_pSettings->waterField->yMax()-m_pSettings->waterField->yMin());
  cenx = m_pSettings->waterField->xMin() + scalex*0.5f;
  ceny = m_pSettings->waterField->yMin() + scaley*0.5f;
  minx = m_pSettings->waterField->xMin();
  miny = m_pSettings->waterField->yMin();
  minscale = scalex < scaley ? scalex : scaley;
  maxscale = scalex > scaley ? scalex : scaley;

  minTime = 750;
  maxTime = 1400;

  load();
}

void AnimationEffect::reset()
{
  startFrame = m_pSettings->frame;
  for (int i = 0; i < MAX_COLORS; i++)
    palette[i] = randColor();

  start();
}

void EffectRain::start()
{
  rainDensity = 2.0f + 30.0f * frand();
}
void EffectRain::apply()
{
  if (frand() < rainDensity/60.f)
    m_pSettings->waterField->SetHeight(minx+frand()*scalex,miny+frand()*scaley,0.5f+0.5f*frand(),-2.0f-frand()*2, randColor());
}

void EffectSwirl::start()
{
  invertSwirls = (rand() % 2) == 0;
  swirlImages = rand() % 3 + 1;
  if (invertSwirls) swirlImages *= 2;
}
void EffectSwirl::apply()
{
  D3DXVECTOR3 pos;
  D3DXMATRIX mat;
  for (int i = 0; i < swirlImages; i++)
  {
    float offset = (float)i * 2*3.14159f / (float)swirlImages;
    D3DXMatrixRotationYawPitchRoll(&mat, 0,0,offset);
    pos.z = 3.0f+2.5f*sin(m_pSettings->frame*0.007f+offset);
    //Pretty sinusoidal swimming
    pos.x = minx + scalex/2.0f + (float)7.0f/20.0f*minscale*sin(m_pSettings->frame*0.035f);
    pos.y = miny + scaley/2.0f +(float)7.0f/20.0f*minscale*cos(m_pSettings->frame*0.045f);
    TransformCoord(&pos,&pos,&mat);
    m_pSettings->waterField->SetHeight(pos.x,pos.y,2.5f,-2.5f*(invertSwirls ? (i%2)*2-1:1), palette[i]);
  }
}

void EffectTwist::start()
{
  speed = 0.15f + 0.07f * frand();
  rotate = 0.001f + 0.005f*frand();
  count = rand() % 4 +1;
  modulate = 0.001f + 0.005f*frand();
}
void EffectTwist::apply()
{
  D3DXVECTOR3 posO, posC, pos;
  D3DXMATRIX mat;
  for (int i = 0; i < count; i++)
  {
    float offset = (float)i * 2*3.14159f / (float)count;
    D3DXMatrixRotationYawPitchRoll(&mat, 0,0,m_pSettings->frame*rotate + offset);
    posC.x = 0.4f*minscale * sin(m_pSettings->frame*modulate);
    posC.y = 0;
    TransformCoord(&posC,&posC,&mat);
    
    D3DXMatrixRotationYawPitchRoll(&mat, 0,0,m_pSettings->frame*speed);
    posO.x = minscale/36.5f;
    posO.y = minscale/36.5f;
    TransformCoord(&posO,&posO,&mat);
    pos.x = posC.x+posO.x;
    pos.y = posC.y+posO.y;
    m_pSettings->waterField->SetHeight(posC.x+posO.x,posC.y+posO.y,1.0,-2.5,palette[0+2*i]);
    m_pSettings->waterField->SetHeight(posC.x-posO.x,posC.y-posO.y,1.0,-2.5,palette[1+2*i]);
  }
}

void EffectXBMCLogo::start()
{
  minTime = 200;
  maxTime = 201;
}
void EffectXBMCLogo::apply()
{
  int time = m_pSettings->frame - startFrame;
  int times[] = {50, 100,145};
  float r,x,y;
  float ms = minscale, cx = cenx, cy = ceny;
  float sx = m_pSettings->scaleX;//1.0f;//m_pSettings->widescreen ? 9.0f/480.0f*720.0f/16.0f : 1;

  if (time < times[0])
  {
    float r = time/(float)times[0];
    //D3DXVECTOR3 posAs = D3DXVECTOR3(cx+ms*-0.402f, cy+ms*-0.133f,0);
    //D3DXVECTOR3 posAe = D3DXVECTOR3(cx+ms*0.444f,  cy+ms*-0.080f,0);
    D3DXVECTOR3 posAs = D3DXVECTOR3(cx+sx*ms*-0.502f, cy+ms*-0.133f,0);
    D3DXVECTOR3 posAe = D3DXVECTOR3(cx+sx*ms*0.544f,  cy+ms*-0.080f,0);
    D3DXVECTOR3 posBs = D3DXVECTOR3(cx+sx*ms*-0.262f, cy+ms*0.291f,0);
    D3DXVECTOR3 posBe = D3DXVECTOR3(cx+sx*ms*0.350f,  cy+ms*0.223f,0);

    m_pSettings->waterField->DrawLine(
      posAe.x*r + posAs.x*(1-r), posAe.y*r + posAs.y*(1-r),
      posBe.x*r + posBs.x*(1-r), posBe.y*r + posBs.y*(1-r),
      2.0, 1.4f,0.05f,D3DCOLOR_RGBA(54,69,102,255));
  }
  
  else if (time < times[1]) //XBMC
  {
    ms *= 0.78f;
    cx += 10.0f/ms;
    cy += 10.0f/ms;

    float rorig = (time-times[0])/(float)(times[1]-times[0]);
    r = rorig*.75f + .125f;
    x = abs(r-0.5f) > 0.25f ? 4*abs(r-0.5f)-1 : cos(r*2*3.141592f);
    y = abs(r-0.5f) > 0.25f ? (r>0.5f?-1:1) : sin(r*2*3.141592f);
    //x
    m_pSettings->waterField->SetHeight(cx+sx*ms*(-0.45f+(1+x)*0.1f), cy+ms*(-0.0f-y*0.1f),0.6f,0.6f, D3DCOLOR_RGBA(133,194,38,255));
    m_pSettings->waterField->SetHeight(cx+sx*ms*(-0.45f-(1+x)*0.1f), cy+ms*(-0.0f-y*0.1f),0.6f,0.6f, D3DCOLOR_RGBA(133,194,38,255));
    //c
    m_pSettings->waterField->SetHeight(cx+sx*ms*(0.46f+x*0.1f), cy+ms*(-0.0f+y*0.1f),0.6f,0.6f, D3DCOLOR_RGBA(189,209,227,255));
  
    r = rorig*1.25f;
    x = r < 0.25f ? -1 : r>1.0f? 4*(1.0f-r): sin(-r*2*3.141592f);
    y = r < 0.25f ? 2 - 8*r :r>1.0f? 1.0f: cos(-r*2*3.141592f);
    //b
    m_pSettings->waterField->SetHeight(cx+sx*ms*(-0.180f+x*0.1f), cy+ms*(-0.00f-y*0.1f),0.6f,0.6f, D3DCOLOR_RGBA(133,194,38,255));
  
    r = rorig * 0.75f;
    x = r < 0.25f ? -1 : sin(-r*2*3.141592f);
    y = r < 0.25f ? 1 - 4*r : cos(-r*2*3.141592f);
    //m
    m_pSettings->waterField->SetHeight(cx+sx*ms*(0.04f+x*0.1f), cy+ms*(-0.0f+y*0.1f),0.6f,0.6f, D3DCOLOR_RGBA(189,209,227,255));
    m_pSettings->waterField->SetHeight(cx+sx*ms*(0.24f+x*0.1f), cy+ms*(-0.0f+y*0.1f),0.6f,0.6f, D3DCOLOR_RGBA(189,209,227,255));
    if (r < 0.25) m_pSettings->waterField->SetHeight(cx+sx*ms*(0.44f+x*0.1f), cy+ms*(-0.0f+y*0.1f),0.6f,0.6f, D3DCOLOR_RGBA(189,209,227,255));
  }
  else if (time < times[2])
  {
    float r = (time-times[1])/(float)(times[2]-times[1]);
    x = r;
    float size = 0.35f + 0.4f*(1.0f-r)*(1.0f-r);
    m_pSettings->waterField->SetHeight(cx+sx*ms*(0.4f-.8f*x), cy+ms*(0.253f),size,size, D3DCOLOR_RGBA(217,130,46,255));
  }
}

void EffectBoil::start()
{
  boilingDensity = 0.016f;
  for (int i = 0; i < NUM_BUBBLES; i++)
    popBubble(&bubbles[i]);
}
void EffectBoil::apply()
{
  incrementBubbles();
  drawBubbles();
}

void EffectBoil::popBubble(Bubble * bub)
{
  bub->alive = false;
  bub->x = 0.0f;
  bub->y = 0.0f;
  bub->size = 0.0f;
  bub->speed = 0.0f;
}
bool EffectBoil::bubblesTooClose(Bubble * bubbleA, Bubble * bubbleB)
{
  float distsq = (bubbleA->x - bubbleB->x)*(bubbleA->x - bubbleB->x) + 
    (bubbleA->y - bubbleB->y)*(bubbleA->y - bubbleB->y); 
  return (bubbleA->size + bubbleB->size)*(bubbleA->size + bubbleB->size) > distsq;
}

void EffectBoil::combineBubbles(Bubble * bubbleA, Bubble * bubbleB)
{
  Bubble * dst = bubbleB, * src = bubbleA;
  if (bubbleA->size > bubbleB->size)
  {
    dst = bubbleA;
    src = bubbleB;
  }
  if (src->size == 0)
  {
    src->alive = false;
    return;
  }
  float ratio = dst->size / (dst->size + src->size);
  dst->size = (float)pow((double)(dst->size*dst->size*dst->size+ src->size*src->size*src->size),0.33333);
  dst->x = dst->x * ratio + src->x*(1-ratio);
  dst->y = dst->y * ratio + src->y*(1-ratio);
  popBubble(src);
}

void EffectBoil::incrementBubbles()
{
  for (int i = 0; i < NUM_BUBBLES; i++)
  {
    if (!bubbles[i].alive)
    {
      if (frand() < boilingDensity)
      {
        bubbles[i].alive = true;
        bubbles[i].x = minx + scalex*frand();
        bubbles[i].y = miny + scaley*frand();
        bubbles[i].size = 0.0f;
        bubbles[i].speed = 0.05f + 0.1f*frand();
      }
    }
    else 
    {
      bubbles[i].size += bubbles[i].speed;
      for (int j = 0; j < i; j++)
      {
        if (bubbles[j].alive && bubblesTooClose(&bubbles[i],&bubbles[j]))
          combineBubbles(&bubbles[i],&bubbles[j]);
      }
      if (bubbles[i].size > 2.0 && frand() < 0.2)
        popBubble(&bubbles[i]);
      else if (bubbles[i].size > 4.0)
        popBubble(&bubbles[i]);
    }
  }
}
void EffectBoil::drawBubbles()
{
 for (int i = 0; i < NUM_BUBBLES; i++)
  if (bubbles[i].alive)
    m_pSettings->waterField->SetHeight(bubbles[i].x,bubbles[i].y,bubbles[i].size,bubbles[i].size*0.7f, palette[i]);
}

void EffectText::start()
{
  char * strings[6] = {"XBMC","Asteron","Water","2007","Tetris","Hi Mom"};
  strcpy(marqueeString,strings[rand()%6]);
}
void EffectText::apply()
{
  D3DXVECTOR3 pos;
  pos.x = minx + scalex/2.0f + (float)7.0f/20.0f*minscale*sin(m_pSettings->frame*0.015f);
  pos.y = miny + scaley/2.0f +(float)6.0f/20.0f*minscale*cos(m_pSettings->frame*0.025f);
  drawString(marqueeString,1.0f,1.5f,2.3f,0.2f,pos.x-5.0f,pos.y);
}

void EffectText::drawLine(float xa, float ya, float xb, float yb, float width)
{
  m_pSettings->waterField->DrawLine(xa,ya,xb,yb,width,0.4f,0.5f,randColor());
}


int segmentdisplay[37][16] ={
  {1,1,1,0,0,0,1,1,1,1,0,0,0,1,0,0}, //A
  {1,0,1,0,1,0,0,1,1,1,0,0,0,1,1,1}, //B
  {1,1,1,0,0,0,0,0,0,1,0,0,0,0,1,1}, //C
  {1,1,0,0,1,0,1,0,0,0,0,1,0,1,1,1}, //D
  {1,1,1,0,0,0,0,1,0,1,0,0,0,0,1,1}, //E
  {1,1,1,0,0,0,0,1,0,1,0,0,0,0,0,0}, //F
  {1,1,1,0,0,0,0,0,1,1,0,0,0,1,1,1}, //G
  {0,0,1,0,0,0,1,1,1,1,0,0,0,1,0,0}, //H
  {1,1,0,0,1,0,0,0,0,0,0,1,0,0,1,1}, //I
  {0,0,0,0,0,0,1,0,0,1,0,0,0,1,1,1}, //J
  {0,0,1,0,0,1,0,1,0,1,0,0,1,0,0,0}, //K
  {0,0,1,0,0,0,0,0,0,1,0,0,0,0,1,1}, //L
  {0,0,1,1,0,1,1,0,0,1,0,0,0,1,0,0}, //M
  {0,0,1,1,0,0,1,0,0,1,0,0,1,1,0,0}, //N
  {1,1,1,0,0,0,1,0,0,1,0,0,0,1,1,1}, //O
  {1,1,1,0,0,0,1,1,1,1,0,0,0,0,0,0}, //P
  {1,1,1,0,0,0,1,0,0,1,0,0,1,1,1,1}, //Q
  {1,1,1,0,0,0,1,1,1,1,0,0,1,0,0,0}, //R
  {1,1,1,0,0,0,0,1,1,0,0,0,0,1,1,1}, //S
  {1,1,0,0,1,0,0,0,0,0,0,1,0,0,0,0}, //T
  {0,0,1,0,0,0,1,0,0,1,0,0,0,1,1,1}, //U
  {0,0,1,0,0,1,0,0,0,1,1,0,0,0,0,0}, //V
  {0,0,1,0,0,0,1,0,0,1,1,0,1,1,0,0}, //W
  {0,0,0,1,0,1,0,0,0,0,1,0,1,0,0,0}, //X
  {0,0,0,1,0,1,0,0,0,0,0,1,0,0,0,0}, //Y
  {1,1,0,0,0,1,0,0,0,0,1,0,0,0,1,1}, //Z
  {1,1,1,0,0,0,1,0,0,1,0,0,0,1,1,1}, //0
  {0,0,0,0,1,0,0,0,0,0,0,1,0,0,0,0}, //1
  {1,1,0,0,0,0,1,1,1,1,0,0,0,0,1,1}, //2
  {1,1,0,0,0,0,1,1,1,0,0,0,0,1,1,1}, //3
  {0,0,1,0,0,0,1,1,1,0,0,0,0,1,0,0}, //4
  {1,1,1,0,0,0,0,1,1,0,0,0,0,1,1,1}, //5
  {1,1,1,0,0,0,0,1,1,1,0,0,0,1,1,1}, //6
  {1,1,0,0,0,1,0,0,0,0,1,0,0,0,0,0}, //7
  {1,1,1,0,0,0,1,1,1,1,0,0,0,1,1,1}, //8
  {1,1,1,0,0,0,1,1,1,0,0,0,0,1,0,0}, //9
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}  //space
};
void EffectText::drawChar(char c, float sizex, float sizey, float width, float posx, float posy)
{
  float xa = posx, xb = posx + sizex/2, xc = posx + sizex;
  float ya = posy, yb = posy + sizey/2, yc = posy + sizey;

  if (c >= 'A' && c <= 'Z') c = c - 'A';
  else if (c >= 'a' && c <= 'z') c = c - 'a';
  else if (c >= '0' && c <= '9') c = 26 + c - '0';
  else c = 36;

  if (segmentdisplay[c][ 0]) drawLine(xa,ya,xb,ya,width);
  if (segmentdisplay[c][ 1]) drawLine(xb,ya,xc,ya,width);
  if (segmentdisplay[c][ 2]) drawLine(xa,ya,xa,yb,width);
  if (segmentdisplay[c][ 3]) drawLine(xa,ya,xb,yb,width);
  if (segmentdisplay[c][ 4]) drawLine(xb,ya,xb,yb,width);
  if (segmentdisplay[c][ 5]) drawLine(xc,ya,xb,yb,width);
  if (segmentdisplay[c][ 6]) drawLine(xc,ya,xc,yb,width);
  if (segmentdisplay[c][ 7]) drawLine(xa,yb,xb,yb,width);
  if (segmentdisplay[c][ 8]) drawLine(xb,yb,xc,yb,width);
  if (segmentdisplay[c][ 9]) drawLine(xa,yb,xa,yc,width);
  if (segmentdisplay[c][10]) drawLine(xb,yb,xa,yc,width);
  if (segmentdisplay[c][11]) drawLine(xb,yb,xb,yc,width);
  if (segmentdisplay[c][12]) drawLine(xb,yb,xc,yc,width);
  if (segmentdisplay[c][13]) drawLine(xc,yb,xc,yc,width);
  if (segmentdisplay[c][14]) drawLine(xa,yc,xb,yc,width);
  if (segmentdisplay[c][15]) drawLine(xb,yc,xc,yc,width);
}

void EffectText::drawString(char * sz, float spacing, float sizex, float sizey, float width, float posx, float posy)
{
  for (int i = 0; sz[i] != 0; i++)
    drawChar(sz[i], sizex, sizey, width, posx + i * (sizex + spacing), posy );
}

void EffectBullet::start()
{
  bulletDensity = 0.0016f;
  minsize = 0.8f + 0.4f*frand();
  maxsize = 1.7f + 0.5f*frand();
  for (int i = 0; i < NUM_BULLETS; i++)
    resetBullet(&bullets[i]);
}
void EffectBullet::apply()
{
  incrementBullets();
  drawBullets();
}

void EffectBullet::resetBullet(Bullet * Bullet)
{
  Bullet->alive = false;
  Bullet->x = 0.0f;
  Bullet->y = 0.0f;
  Bullet->dx = 0.0f;
  Bullet->dy = 0.0f;
  Bullet->size = 0.0f;
  Bullet->speed = 0.0f;
  Bullet->deadTime = 0;
}
bool EffectBullet::bulletsTooClose(Bullet * bulletA, Bullet * bulletB)
{
  float distsq = (bulletA->x - bulletB->x)*(bulletA->x - bulletB->x) + 
    (bulletA->y - bulletB->y)*(bulletA->y - bulletB->y); 
  return (bulletA->size + bulletB->size)*(bulletA->size + bulletB->size) > distsq;
}

int EffectBullet::timeToHit(Bullet * bul)
{
  int ta, tb;
  ta = (int)(((minx + (bul->dx>0?scalex:0))- bul->x)/(bul->dx*bul->speed));
  tb = (int)(((miny + (bul->dy>0?scaley:0))- bul->y)/(bul->dy*bul->speed));
  return iMin(ta,tb);
  //return 100;
}

void EffectBullet::incrementBullets()
{
  int i;
  for (i = 0 ; i < NUM_BULLETS; i++)
    if (bullets[i].alive)
    {
      bullets[i].x += bullets[i].dx * bullets[i].speed;
      bullets[i].y += bullets[i].dy * bullets[i].speed;
    }
  for (i = 0; i < NUM_BULLETS; i++)
  {
    if (!bullets[i].alive)
    {
      if (frand() < bulletDensity)
      {
        bullets[i].speed = 0.2f + 0.3f*frand();
        bullets[i].size = minsize + (maxsize - minsize)*frand();

        float angle = frand()*2*3.141592f;
        bullets[i].alive = true;

        bullets[i].dx = sin(angle);
        bullets[i].dy = cos(angle);
        bullets[i].x = minx + scalex*frand();
        bullets[i].y = miny + scaley*frand();
        
        int time = timeToHit(&bullets[i]);
        bullets[i].x += bullets[i].dx * time;
        bullets[i].y += bullets[i].dy * time;
        bullets[i].dx *= -1;
        bullets[i].dy *= -1;
        bullets[i].deadTime = m_pSettings->frame + timeToHit(&bullets[i]);
      }
    }
    else 
    {
      for (int j = 0; j < i; j++)
        if (bullets[j].alive && bulletsTooClose(&bullets[i],&bullets[j]))
          bounceBullets(&bullets[i],&bullets[j]);
    
      if (bullets[i].deadTime <= m_pSettings->frame)
        resetBullet(&bullets[i]);
    }
  }
}
void EffectBullet::bounceBullets(Bullet * bulA, Bullet * bulB)
{
  float  m21,dvx2,a,x21,y21,vx21,vy21,fy21;

  //m21=bulB->size/bulA->size;
  m21=bulB->size/bulA->size*bulB->size/bulA->size;
  x21=bulB->x - bulA->x;
  y21=bulB->y - bulA->y;
  vx21=bulB->dx * bulB->speed - bulA->dx * bulA->speed;
  vy21=bulB->dy * bulB->speed - bulA->dy * bulA->speed;

  if (x21*vx21 > 0 && y21*vy21 > 0) // they are moving away from eachother
    return;

//     *** I have inserted the following statements to avoid a zero divide; 
//         (for single precision calculations, 
//          1.0E-12 should be replaced by a larger value). **************  

  fy21= .0000001f*fabs(y21);                            
  if ( fabs(x21)<fy21 )   
    x21=fy21 * (x21<0?-1:1); 
  

//     ***  update velocities ***
  a=y21/x21;
  dvx2= -2*(vx21 +a*vy21)/((1+a*a)*(1+m21)) ;
  
  bulB->dx = bulB->dx*bulB->speed +  dvx2;
  bulB->dy = bulB->dy*bulB->speed + a*dvx2;
  bulB->speed = sqrt(bulB->dx*bulB->dx + bulB->dy*bulB->dy);
  bulB->dx /= bulB->speed;
  bulB->dy /= bulB->speed;
  bulB->speed = bulB->speed < 0.15f ? 0.15f : bulB->speed > 0.6f ? 0.6f : bulB->speed;

  bulA->dx = bulA->dx*bulA->speed - m21*dvx2;
  bulA->dy = bulA->dy*bulA->speed - a*m21*dvx2;
  bulA->speed = sqrt(bulA->dx*bulA->dx + bulA->dy*bulA->dy);
  bulA->dx /= bulA->speed;
  bulA->dy /= bulA->speed;
  bulA->speed = bulA->speed < 0.15f ? 0.15f : bulA->speed > 0.6f ? 0.6f : bulA->speed;

  bulA->deadTime = m_pSettings->frame + timeToHit(bulA);
  bulB->deadTime = m_pSettings->frame + timeToHit(bulB);

  return;
}

void EffectBullet::drawBullets()
{
  float scale = 1.35f;
  for (int i = 0; i < NUM_BULLETS; i++)
    if (bullets[i].alive)
      m_pSettings->waterField->SetHeight(bullets[i].x,bullets[i].y,bullets[i].size*scale,bullets[i].size*scale*0.8f, palette[i]);
}

