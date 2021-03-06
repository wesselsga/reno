#include "server.h"
#include "channel.h"
#include "config.h"
#include "../version.h"

#include "clock.h"
#include "window.h"

#include "image.h"

#ifndef _WIN32
#include <unistd.h>
#endif

void prompt()
{
#ifdef _WIN32
	//::MessageBox(0, 
	//		L"Attach debugger?", 
	//		L"Debugger", 
	//		MB_YESNO|MB_ICONQUESTION);
#endif
}

void wait_for_a_while(uv_idle_t* handle) 
{
   std::chrono::milliseconds timeout(10);
   std::this_thread::sleep_for(timeout);
}

Channel::Channel(const _priv&, 
		const std::string& id) 
	: _id(id)
{
	_iothread = 0;

	uv_mutex_init(&_lock);
	uv_cond_init(&_isReady);
}
	
Channel::~Channel()
{
	uv_cond_destroy(&_isReady);
	uv_mutex_destroy(&_lock);
}

int32_t Channel::run(const std::string& id)
{
	prompt();
   
   auto conf = Config::open(); 
   if (!conf)
   {
      LOG(ERR) << "channel[" << id << "] invalid config!";
      return -1;
   }

   // now we have config - get our 
   // corresponding block within the conf
   auto block = conf->getChannel(id);
   if (!block)
   {
      LOG(ERR) << "channel[" << id << "] missing config!";
      return -1;
   }

	auto channel = std::make_shared<Channel>(_priv{}, id);
	
	uv_thread_create(
			&channel->_iothread, 
			iothread, 
			channel.get());

	uv_cond_wait(&channel->_isReady, &channel->_lock);

	LOG(VERBOSE) << "channel[" << id << "] opening ...";	
	
	gfx::startup();		

	{
		std::shared_ptr<gfx::Context> ctx;

      std::stringstream title;
      title << PRODUCT << " (v" << VERSION_STR << ") | " << "channel:" << channel->id();
      
      auto window = screen::Window::create(
               title.str(), 
               block->width(), 
               block->height(), 
               channel);

      ctx = gfx::Context::create(
            window->display(), 
            window->handle());
      
      // create the video mixer
      gfx::Compositor compositor(Clock::create());

      auto home = Path::home();
      home = home.append(PRODUCT);
      home.mkdir();
      home = home.append("startup");
      
      std::vector<std::string> files;
      home.list(files);
      for (auto f : files)
      {
         LOG(VERBOSE) << "adding layer for " << f;
            
         Path filename = home.append(f);
         auto stream = gfx::Image::create(filename.c_str());
         if (stream)
         {
            compositor.addLayer(stream);
         }
      }     

      auto layer = compositor.layer(1);
      layer->setVisible(false);

      // show the window on screen
		window->show();
		window->update();

		LOG(DEBUG) << "channel[" << id << "] starting rendering loop ...";

		uint32_t frame = 0;

		int64_t freq = hires_frequency();
		int64_t now, mark = hires_time();

      bool fx = false;
      int64_t timer = hires_time();

		for (;;)
		{
         if (window->pump() < 0)
			{
				channel->close();
				break;
			}

         if (!fx && ((hires_time() - timer) > (hires_frequency()*2)))
         {
            //compositor.addEffect("fadeout", 1000, 0);
            compositor.addEffect("fadein", "slow", 1);
            layer = compositor.layer(1);
            fx = true;
         }


         ctx->clear();
         
         compositor.process();

			ctx->swapBuffers();	
			++frame;

			now = hires_time();
			if (((now - mark) / freq) > 1)
			{
				double elapsed = 
						static_cast<double>((now - mark) / (freq*1.0));

				double fps = frame / elapsed;
				LOG(VERBOSE) << "channel[" << channel->id() << "]: fps=" 
						<< std::fixed << std::setprecision(3)
						<< fps;

				frame = 0;
				mark = hires_time();
			}

         //std::chrono::milliseconds timeout(1);
         //std::this_thread::sleep_for(timeout);
         
		}
	}

	gfx::shutdown();
	
	return 0;
}

void Channel::close()
{
	LOG(INFO) << "channel: " << id() << " closing ...";
	
	//_closed = true;
	
	uv_stop(uv_default_loop());
}

void Channel::iothread(void* arg) 
{
	Channel* self = reinterpret_cast<Channel*>(arg);
	if (!self){
		return;
	}

	uv_pipe_t pipe = {0};

	auto loop = uv_default_loop();

	if (uv_guess_handle(1) == UV_NAMED_PIPE)
	{
		trace::clearWriters();

		uv_pipe_init(loop, &pipe, 0);
		uv_pipe_open(&pipe, 1);
		uv_unref((uv_handle_t*)&pipe);
		
      auto writer = 
            std::make_shared<trace::UvWriter>((uv_stream_t*)&pipe);
		trace::addWriter(":pipe", writer);
	}

	// signal the thread is up and running
	uv_cond_signal(&self->_isReady);

	uv_idle_t idler;
	uv_idle_init(loop, &idler);
	uv_idle_start(&idler, wait_for_a_while);

	uv_run(loop, UV_RUN_DEFAULT);
}
