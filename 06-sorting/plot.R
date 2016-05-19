###################################################################################### plot.R
## Brooklyn College CISC3130 M. Lowenthal  - Assignment #6
## Robert Wagner
## 2016-05-18
##
## This is an R script to generate plots based on the output of the sort analysis
## reads 5 replications of the analysis data
## And saves them to PDF format
####################################################################################
library(ggplot2)
df1 <- read.csv("~/courses/CISC3130/06-sorting/small1.txt", header=TRUE)
df2 <- read.csv("~/courses/CISC3130/06-sorting/small2.txt", header=TRUE)
df3 <- read.csv("~/courses/CISC3130/06-sorting/small3.txt", header=TRUE)
df4 <- read.csv("~/courses/CISC3130/06-sorting/small4.txt", header=TRUE)
df5 <- read.csv("~/courses/CISC3130/06-sorting/small5.txt", header=TRUE)
df <- rbind(df1, df2, df3, df4, df5)

time_plot <- ggplot(df, 
  aes(x = size, y=ms_elapsed / 1000, color = sort)) +
  geom_smooth() +
  facet_wrap(~ initial_order) +
  scale_y_continuous('Milli-Seconds Elapsed (log2)', trans="log2") +
  scale_x_continuous('Size of Dataset') + 
  scale_color_grey(start = 0.8, end = 0) +
  theme_bw() +
  theme(axis.text.y = element_text(angle = 45), 
        axis.text.x = element_text(angle = 45, hjust = 1),
        legend.position = c(1,0),
        legend.justification = c(1,0),
        legend.key = element_rect(fill = NA, color = NA, size = 0.25)
  ) +
  ggtitle("Time elapsed vs dataset size of various sorts and various initial orders")

exchanges_plot <- ggplot(df,
  aes(x = size, y = exchanges, color = sort)) +
  geom_smooth() +
  facet_wrap(~ initial_order) +
  scale_y_continuous('Exchanges Performed') +
  scale_x_continuous('Size of Dataset') + 
  scale_color_grey(start = 0.8, end = 0) +
  theme_bw() +
  theme(axis.text.y = element_text(angle = 45), 
        axis.text.x = element_text(angle = 45, hjust = 1),
        legend.position = c(1,0),
        legend.justification = c(1,0),
        legend.key = element_rect(fill = NA, color = NA, size = 0.25)
  ) +
  ggtitle("Exchanges vs dataset size of various sorts and various initial orders")

compares_plot <- ggplot(df,
  aes(x = size, y = compares, color = sort)) +
  geom_smooth() +
  facet_wrap(~ initial_order) +
  scale_y_continuous('Comparisons Performed (log2)', trans="log2") +
  scale_x_continuous('Size of Dataset') + 
  scale_color_grey(start = 0.8, end = 0) +
  theme_bw() +
  theme(axis.text.y = element_text(angle = 45), 
        axis.text.x = element_text(angle = 45, hjust = 1),
        legend.position = c(1,0),
        legend.justification = c(1,0),
        legend.key = element_rect(fill = NA, color = NA, size = 0.25)
  ) +
  ggtitle("Comparisons vs dataset size of various sorts and various initial orders")

ggsave(filename = "~/courses/CISC3130/06-sorting/plot_time.pdf", 
       plot = time_plot, 
       width = 10, 
       height = 7.5)
ggsave(filename = "~/courses/CISC3130/06-sorting/plot_compares.pdf", 
       plot = compares_plot, 
       width = 10, 
       height = 7.5)
ggsave(filename = "~/courses/CISC3130/06-sorting/plot_exchanges.pdf", 
       plot = exchanges_plot, 
       width = 10, 
       height = 7.5)


